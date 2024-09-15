#!/bin/bash
set -e

# Clone the repository
clone_repo() {
    if [ ! -d "portal_turret_webpage" ]; then
        git clone https://github.com/joranderaaff/portal_turret_webpage.git
    fi
}

# Install npm dependencies
install_dependencies() {
    if [ ! -d "portal_turret_webpage/node_modules" ]; then
        clone_repo
        cd portal_turret_webpage
        npm install
        cd ..
    fi
}

# Build the project
build_project() {
    install_dependencies
    cd portal_turret_webpage
    npm run build
    cp dist_compiled/index.html.gz.h ../generated/
    cd ..
}

# Clean up the project
distclean() {
    rm -rf portal_turret_webpage
}

# Extract audio from portal 2 data
audio() {
    PORTAL2_DIR="c:/Program Files (x86)/Steam/steamapps/common/Portal 2"
    cd audio
    if [ ! -e ffmpeg ]; then
      curl -L https://www.gyan.dev/ffmpeg/builds/ffmpeg-release-essentials.zip -o ffmpeg.zip
      unzip ffmpeg.zip
      mv ffmpeg-* ffmpeg
      rm ffmpeg.zip
    fi
    rm -rf out
    mkdir -p out
    export PATH=$PATH:`pwd`/ffmpeg/bin
    if [ "$1" = "all" ]; then
      python extract.py "${PORTAL2_DIR}/portal2/pak01_dir.vpk"
      mv out english
      for i in german french spanish russian; do
        mkdir out
        echo will extract $i language
        read
        python extract.py "${PORTAL2_DIR}/portal2_$i/pak01_dir.vpk" "${PORTAL2_DIR}/portal2/pak01_dir.vpk"
        mv out $i
      done
    elif [ "$1" == "" -o "$1" == "english" ]; then
      python extract.py "${PORTAL2_DIR}/portal2/pak01_dir.vpk"
    else
      python extract.py "${PORTAL2_DIR}/portal2_$1/pak01_dir.vpk" "${PORTAL2_DIR}/portal2/pak01_dir.vpk"
    fi
}

download_mklittlefs() {
    if [ ! -e mkfsimg.py ]; then
        curl https://raw.githubusercontent.com/jrast/littlefs-python/0.4.0/examples/mkfsimg.py | sed 's#relpath = os.path.relpath(path, start=source_dir)#relpath = "/" + os.path.relpath(path, start=source_dir).replace("\\\\", "/")#' > mkfsimg.py
    fi
}

# Download littlefs partition
fs_get() {
    rm -rf fs
    pip install littlefs-python==0.12.0
    OFFSET=`cat partitions.csv  | grep spiffs | awk -F, '{print $4}'`
    SIZE=`cat partitions.csv | grep spiffs | awk -F, '{print $5}'`
    esptool.py read_flash $OFFSET $SIZE fs.bin
    littlefs-python extract fs.bin fs --block-size=4096
    rm fs.bin
}

# Upload littlefs partition
fs_put() {
    pip install littlefs-python==0.4.0
    OFFSET=`cat partitions.csv  | grep spiffs | awk -F, '{print $4}'`
    OFFSET=`printf "%d" $OFFSET`
    SIZE=`cat partitions.csv | grep spiffs | awk -F, '{print $5}'`
    SIZE=`printf "%d" $SIZE`
    python mkfsimg.py --img-filename fs.bin --img-size $SIZE --block-size 4096 fs
    esptool.py write_flash $OFFSET fs.bin
    rm fs.bin
}

# Main function to control the flow
main() {
    case "$1" in
        distclean)
            distclean
            ;;
        audio)
            shift
            audio $*
            ;;
        fs_get)
            fs_get
            ;;
        fs_put)
            download_mklittlefs
            fs_put
            ;;
        *)
            build_project
            ;;
    esac
}

# Execute main function with passed arguments
main "$@"