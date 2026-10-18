"""Summarize an owned AE25.6 lifecycle trial; this does not certify Auto."""
import argparse
import collections
import json
import re
from pathlib import Path


def summarize(root):
    report = (root / 'host-report.txt').read_text(errors='replace')
    trace = (root / 'process-error.log').read_text(errors='replace')
    if 'PASS color parameter experiment renders' not in report or 'FAIL ' in report:
        raise ValueError('Owned color experiment did not complete')
    begin = trace.index('FSTR_RUNNER executing owned script')
    end = trace.index('FSTR_RUNNER return', begin)
    window = trace[begin:end]
    selectors = collections.defaultdict(collections.Counter)
    for command, main in re.findall(r'FSTR_COLOR_COMMAND cmd=(\d+) main=(\d+)', window):
        selectors[command][main] += 1
    stages = collections.defaultdict(set)
    for stage, index in re.findall(r'FSTR_COLOR_PROBE main=\d+ stage=(\d+) index=(\d+)', window):
        stages[int(stage)].add(int(index))
    if set(stages) != set(range(1, 17)) or any(v != set(range(1, 7)) for v in stages.values()):
        raise ValueError('Missing render-stage observations')
    idle = window.count('FSTR_IDLE ')
    queue = dict(collections.Counter(re.findall(r'FSTR_RQM event=(\w+)', window)))
    return {
        'result': 'PASS_DIAGNOSTIC_ONLY',
        'stages': 16,
        'idle_callbacks_inside_script': idle,
        'render_queue_registration_succeeded': 'FSTR_RQM register=0 release=0' in trace,
        'render_queue_events_inside_script': queue,
        'selectors_by_main_thread': {k: dict(v) for k, v in selectors.items()},
        'idle_route_rejected_for_this_trial': idle == 0,
        'queue_route_not_observed_for_this_trial': not queue,
        'auto_acceptance': 'NOT_ESTABLISHED',
        'limits': 'One AE25.6 build/path; sequence transport and queue notifications do not establish current project color metadata.',
    }


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('root', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    result = summarize(args.root)
    with args.output.open('x') as output:
        json.dump(result, output, indent=2)
        output.write('\n')
    print(result['result'])
