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
SHOTS="$ROOT/docs/screenshots"
"$BUILD/zamin_snapshot" "$RES/viewport_phone.png" 1080 1950 editor
"$BUILD/zamin_snapshot" "$RES/viewport_tablet.png" 1560 1100 editor
"$BUILD/zamin_snapshot" "$RES/viewport_play_phone.png" 1080 1950 play
"$BUILD/zamin_snapshot" "$RES/viewport_gizmos_tablet.png" 1560 1100 light
cp "$RES/viewport_tablet.png" "$SHOTS/engine_frame.png"
"$BUILD/zamin_snapshot" "$SHOTS/engine_gizmos.png" 1200 800 light
"$BUILD/zamin_snapshot" "$SHOTS/engine_rotated.png" 1200 800 rotated
"$BUILD/zamin_snapshot" "$SHOTS/engine_play.png" 1200 800 play
"$BUILD/zamin_snapshot" "$SHOTS/engine_2d.png" 1200 800 2d

cd "$ROOT"
./gradlew :editor:recordPaparazziDebug
IMG=android/editor/src/test/snapshots/images
cp $IMG/dev.zamin.editor_PhoneScreenshotTest_editor.png "$SHOTS/editor_phone.png"
cp $IMG/dev.zamin.editor_PhoneScreenshotTest_play.png "$SHOTS/editor_phone_play.png"
cp $IMG/dev.zamin.editor_PhoneScreenshotTest_assets.png "$SHOTS/editor_phone_assets.png"
cp $IMG/dev.zamin.editor_TabletScreenshotTest_editor.png "$SHOTS/editor_tablet.png"
cp $IMG/dev.zamin.editor_TabletScreenshotTest_gizmos.png "$SHOTS/editor_tablet_gizmos.png"
echo "Skrinshotlar: docs/screenshots/"
