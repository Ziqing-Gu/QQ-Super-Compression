# 1.2.3 release checklist

Run Plan C before Plan D. Retain the accepted 1.2.3 Stable Windows build and its verified
hashes. Documentation-only work does not require a duplicate Windows build.

1. Retain both completed 23-page PDF manuals byte-for-byte as explicitly requested. Include their reproducible sources/screenshots,
   bilingual notes and installation guides. Keep the author preface and license.
2. Make a new verified formal source snapshot under the existing Baidu-sync
   hierarchy, including exact JUCE 8.0.15. Never reopen or replace frozen backups.
3. Sync the complete intended source to GitHub, preserve history/workflows, and
   verify main and v1.2.3 match the backed-up production source (Plan C).
4. Build the three Mac jobs from v1.2.3: arm64 VST3, x86_64 VST3, Universal 2 AU.
   Verify runner auval, ZIP integrity, bundle versions, architectures, minimum
   macOS load commands and the actual signature state of each slice.
5. Prepare exactly eight end-user files: root bilingual manuals and guides,
   one Windows ZIP in Win/ and three Mac ZIPs in Mac/. Save the final package
   under D:/Codex/Outputs per the current storage rule. Keep proof/logs outside.
6. Include all eight files in the public 1.2.3 Release; verify local/remote
   sizes and SHA-256 digests, stable download URLs, source tag and Latest status.
7. If release-bound source/docs change again, create another formal snapshot.
   Record final checks externally so completed source snapshots remain frozen.
