# check_all.ps1 — run both Nemi acceptance suites locally (same checks CI
# runs in .github/workflows/ci.yml). Usage:
#
#   .\check_all.ps1
#
# Exits 0 if both suites pass, 1 otherwise. Does not require git/GitHub.

$root = $PSScriptRoot
$failed = $false

Write-Host "== Python acceptance suite ==" -ForegroundColor Cyan
$env:PYTHONUTF8 = "1"
python "$root\python\tests\run_corpus.py"
if ($LASTEXITCODE -ne 0) { $failed = $true }

Write-Host "`n== C++ build + ctest ==" -ForegroundColor Cyan
# clang++ + Ninja is the toolchain verified throughout development; adjust
# -DCMAKE_CXX_COMPILER / -G if your machine uses a different one (MSVC also
# works: drop both flags and let CMake pick the Visual Studio generator).
cmake -S "$root\cpp" -B "$root\cpp\build" -G Ninja -DCMAKE_CXX_COMPILER=clang++
if ($LASTEXITCODE -ne 0) { $failed = $true }

cmake --build "$root\cpp\build"
if ($LASTEXITCODE -ne 0) { $failed = $true }

ctest --test-dir "$root\cpp\build" --output-on-failure
if ($LASTEXITCODE -ne 0) { $failed = $true }

# cpp/build is left in place (already .gitignore'd) so the next run rebuilds
# incrementally instead of reconfiguring from scratch.

if ($failed) {
    Write-Host "`nFAILED — see output above." -ForegroundColor Red
    exit 1
} else {
    Write-Host "`nALL PASSED (Python + C++)." -ForegroundColor Green
    exit 0
}
