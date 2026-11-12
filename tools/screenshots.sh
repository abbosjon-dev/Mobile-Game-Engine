#!/usr/bin/env bash
# Zamin skrinshotlari: engine kadri (offscreen Vulkan) + editor UI (Paparazzi).
# Natija: docs/screenshots/*.png
# Talab: Vulkan drayveri (GPU yoki mesa lavapipe), Android SDK/NDK.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="$ROOT/build/snapshot"
NDK="${ANDROID_NDK:-${ANDROID_HOME:-/opt/android-sdk}/ndk/27.2.12479018}"
RES="$ROOT/android/editor/src/test/resources"

cmake -S "$ROOT/tools/snapshot" -B "$BUILD" -G Ninja -DCMAKE_BUILD_TYPE=Release -DANDROID_NDK="$NDK" >/dev/null
cmake --build "$BUILD"

mkdir -p "$RES" "$ROOT/docs/screenshots"
"$BUILD/zamin_snapshot" "$RES/viewport_phone.png" 1080 1950
"$BUILD/zamin_snapshot" "$RES/viewport_tablet.png" 1560 1100
cp "$RES/viewport_tablet.png" "$ROOT/docs/screenshots/engine_frame.png"

cd "$ROOT"
./gradlew :editor:recordPaparazziDebug
cp android/editor/src/test/snapshots/images/dev.zamin.editor_PhoneScreenshotTest_editor.png docs/screenshots/editor_phone.png
cp android/editor/src/test/snapshots/images/dev.zamin.editor_TabletScreenshotTest_editor.png docs/screenshots/editor_tablet.png
echo "Skrinshotlar: docs/screenshots/"
