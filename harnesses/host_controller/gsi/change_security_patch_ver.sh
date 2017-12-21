#/bin/bash
set -e

function unmount() {
  echo "Unmounting..."
  sudo umount "${MOUNT_POINT}/"
}

SCRIPT_NAME=$(basename $0)
SYSTEM_IMG=$1
OUTPUT_SYSTEM_IMG=$2
NEW_VERSION=$3
FILE_CONTEXTS_BIN=$4

case $# in
  [1,3-4])
    ;;
  *)
    echo "Usage: $SCRIPT_NAME <system.img> (<output_system.img> <new_version_id> (<input_file_contexts.bin>))"
    exit
    ;;
esac

if (("$#" >= 3)) && ((${#NEW_VERSION} != 10)); then
  echo "length of <new_version_id>  must be 10"
  exit
fi

if [ "$ANDROID_BUILD_TOP" == "" ]; then
  echo "Need 'lunch'"
  exit
fi

UNSPARSED_SYSTEM_IMG="${SYSTEM_IMG}.raw"
MOUNT_POINT="${PWD}/temp_mnt"
PROPERTY_NAME="ro.build.version.security_patch"

echo "Unsparsing ${SYSTEM_IMG}..."
simg2img "$SYSTEM_IMG" "$UNSPARSED_SYSTEM_IMG"
IMG_SIZE=$(stat -c%s "$UNSPARSED_SYSTEM_IMG")

echo "Mounting..."
mkdir -p "$MOUNT_POINT"
sudo mount -t ext4 -o loop "$UNSPARSED_SYSTEM_IMG" "${MOUNT_POINT}/"

# check the property file placement
PROPERTY_FILE_PLACES=(
  "/system/build.prop"  # layout of A/B support
  "/build.prop"         # layout of non-A/B support
)
PROPERTY_FILE=""
PLACE=""

echo "Finding build.prop..."
for place in ${PROPERTY_FILE_PLACES[@]}; do
  if [ -f "${MOUNT_POINT}${place}" ]; then
    PROPERTY_FILE="${MOUNT_POINT}${place}"
    PLACE=${place}
    echo "  ${place}"
    break
  fi
done

if [ "$PROPERTY_FILE" != "" ]; then
  if [ "$OUTPUT_SYSTEM_IMG" != "" ]; then
    echo "Replacing..."
  fi
  CURRENT_VERSION=`sudo sed -n -r "s/^${PROPERTY_NAME}=(.*)$/\1/p" ${PROPERTY_FILE}`
  echo "  Current version: ${CURRENT_VERSION}"
  if [[ "$OUTPUT_SYSTEM_IMG" != "" && "$CURRENT_VERSION" != "" ]]; then
    echo "  New version: ${NEW_VERSION}"
    seek=$(sudo grep --byte-offset "${PROPERTY_NAME}=" "${PROPERTY_FILE}" | cut -d':' -f 1)
    seek=$(($seek + ${#PROPERTY_NAME} + 1))   # 1 is for '='
    echo "${NEW_VERSION}" | sudo dd of="${PROPERTY_FILE}" seek="$seek" bs=1 count=10 conv=notrunc
  fi
else
  echo "ERROR: Cannot find build.prop."
fi

if [ "$OUTPUT_SYSTEM_IMG" != "" ]; then
  if [ "$FILE_CONTEXTS_BIN" != "" ]; then
    echo "Writing ${OUTPUT_SYSTEM_IMG}..."

    (cd $ANDROID_BUILD_TOP
     if [[ "$(whereis mkuserimg_mke2fs.sh | wc -w)" < 2 ]]; then
       make mkuserimg_mke2fs.sh -j
     fi
     NON_AB=$(expr "$PLACE" == "/build.prop")
     if [ $NON_AB -eq 1 ]; then
       sudo /bin/bash -c "PATH=out/host/linux-x86/bin/:\$PATH mkuserimg_mke2fs.sh -s ${MOUNT_POINT} $OUTPUT_SYSTEM_IMG ext4 system $IMG_SIZE -D ${MOUNT_POINT} -L system $FILE_CONTEXTS_BIN"
     else
       sudo /bin/bash -c "PATH=out/host/linux-x86/bin/:\$PATH mkuserimg_mke2fs.sh -s ${MOUNT_POINT} $OUTPUT_SYSTEM_IMG ext4 / $IMG_SIZE -D ${MOUNT_POINT}/system -L / $FILE_CONTEXTS_BIN"
     fi)

    unmount
  else
    unmount

    echo "Writing ${OUTPUT_SYSTEM_IMG}..."
    img2simg "$UNSPARSED_SYSTEM_IMG" "$OUTPUT_SYSTEM_IMG"
  fi
else
  unmount
fi

echo "Done."
