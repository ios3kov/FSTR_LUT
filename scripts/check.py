"""Project preflight: real core build and tests; no install, network or publication."""
import argparse, subprocess, sys
from pathlib import Path
def main():
    parser=argparse.ArgumentParser()
    parser.add_argument("--build",default="build-check")
    parser.add_argument("--sanitize",action="store_true")
    args=parser.parse_args();root=Path(__file__).resolve().parents[1]
    build=Path(args.build)
    if build.is_absolute() or ".." in build.parts or len(build.parts)!=1:raise SystemExit("Build must be one local directory name")
    commands=[["cmake","-S",str(root),"-B",str(root/build),"-DCMAKE_BUILD_TYPE=Release",f"-DFSTR_SANITIZE={'ON' if args.sanitize else 'OFF'}"],
              ["cmake","--build",str(root/build),"--parallel","4"],
              ["ctest","--test-dir",str(root/build),"--output-on-failure"]]
    for command in commands:
        completed=subprocess.run(command,cwd=root)
        if completed.returncode:return completed.returncode
    print("PASS: executed pure core contracts. AE, Windows, active ORT cancellation: NOT RUN")
    return 0
if __name__=="__main__":sys.exit(main())
