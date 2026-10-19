# SMART-LOSSLESS-CACHE-001 / SMART-ASSET-HASH-001

Continue from598b590 and experimental1080p repaireab60710ac94a6c9. User authorized safe acceleration and product finishing, no installer/release. Installed5b844 remains protected. Exact accepted image quality, model/provider/normalization settings and memory/disk budgets remain unchanged.

## Evidence and acceptance

The full7second168frame1080p pass computes168native analyses; the next forward MFR pass computes168again. Same keys recur, but the512MiB bounded disk cache only holds128current entries, causing forward-scan eviction. No false cache hit is claimed. Prototype zlib1 compresses the128surviving entries534587104 ->329159726bytes and decodes every byte exactly. Goal is fewer re-runs within the existing budget, not unlimited retention.

C1: Version4cache stores losslessly compressed IEEE32 bytes, with SHA256 of the original uncompressed bytes. Decode only a known expected bounded size; reject truncated/corrupt/trailing/overlong streams, unknown codec and bad checksum. Maximumentry64MiB/resident128MiB/disk512MiB/entrycount4096 remain unchanged. Legacy schemas1/2/3 remain readable. Preserve atomic immutable publication, process lock/recovery and valid-result fallback on persistence failure. Do not touch installed/user cache during tests.
C2: Separate codec/parser negative tests plus real serialized model payload byte parity; no float quantization. Older readers may safely miss version4and recompute; do not call rollback disk reuse certified.
C3: Actual full168frame serial then MFR in owned AE; all outputs <=1PSD code versus repairv5and one another. Prove distinct-thread/frame overlap. Record ready/RAM/disk/compute counts perpass. Qualify retained-mask reuse only if second fullpass does not repeat model inference. Real output export-code equality is separate from full-float equality and continuous motion review.

H1: File verification still reads and hashes **all** bytes at every constructor; no mtime/trusted-path shortcut. Apple CommonCrypto streaming SHA256 must match frozenpicosha2and known vectors, binary/chunk boundaries/empty/error inputs, actual pinned model/runtime assets. Non-Apple portable path retained. No model/session/provider/precision/shape changes. Add helper bytes to build/native-policy bindings so stale code cannot silently share a newanalysispolicy.
H2: Measure alternating legacy/platform hash calls on this Mac independently of host render/build. Separately profile actual constructor stages; session variance cannot be reported as hash speedup or guaranteed total startup improvement.

G1: SDK Release and consistent-vendor ASan+UBSan checks; targeted shared-cache regression and read-only finalreview. G2: real host input/cut/output gates, package inventories/signatures, ownership-safe cleanup, independent final review. Installed5b844 and signed rollback remain untouched until a verified replacement is available and no active AE is using it. No installer/release/push.

Status: prototype evidence only; implementation and host qualification pending. All previous failed attempts retained in sibling ../smart-video-1080p-20261019.

## Implemented and measured

Release SDK30/30, consistent-vendorASan+UBSanSDK30/30, targetedTSan2/2 PASS. Actual C++ codec round-trips all998real serialized v5payloads byte-for-byte and matches their independent SHA256 manifest. Systemzlib/CommonCrypto themselves remain uninstrumented. Separate read-only review found no material defect; it explicitly notes bounded temporary buffers are additional to resident-cache memory.

Diagnostic42d1cc28f6c03933:336full1080p exports,168serial/MFR comparisons and168previous-repair comparisons <=1PSD exportcode. Actual8MFRthreads/141overlapping distinct-frame pairs. All168entries retained at429721178bytes. Second fullpass:0new model calls/168disk reads/654RAM hits. Earlier raw-cache secondpass:168new model calls. Full secondpass169203ms ->89232ms on this exact7second test clip; first serialpass300639ms ->303346ms, so no cold-render speedup claim. This is a bounded-cache workload observation, not playbackFPS or a universal speed guarantee.

Three alternating warm-file pairs per actualscene/model-runtime asset: full SHA digests identical; scene hashing2119.884ms ->145.588ms median, MediaPipe989.927ms ->66.599ms, ORT313.058ms ->21.088ms. Actual owned constructoroneobservation14881.230ms ->11364.737ms, including sceneCoreMLsession10787.543ms ->10722.617ms. Do not attribute session variance to hashing or claim paired total-startup certification.

Canonical6a8957b12ddcf143 installed after CUA normalquit of an unmodified emptyUntitledProject; no save/discard action, allAEprocesses gone checked. Signedexact5b844 rollback preserved in siblingcurrent/previous-installed. Real installed module path observed by lsof. First canonical15exports/15prior5b844 comparisons <=1code,8/16/32and savedCustom18open/save/reopen PASS; fresh native policy safely rebuilt43analyses. SmartOff skipsanalysis; re-enable/strength edit uses5saved masks. First LUT sample also changes time to previously unseen45, so its5newmodels are expected and are not misreported as a LUT invalidation failure or reuse success. Restart test adds a same-frame45prewarm to establish LUT-only reuse correctly, plus original1080p case; pending.

Continuous agent visual playback remainsNOT_RUN; native1080p stills/crops inspected and a7second168frameH264preview is decoded/metadata-checked separately. Windows, playbackFPS, installer, release and redistribution remainoutside this local gate.
