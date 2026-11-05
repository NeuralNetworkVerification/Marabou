#!/bin/bash
set -euo pipefail

curdir=$(pwd)
mydir="${0%/*}"
version=$1
address_model=${2:-64}
case "$address_model" in
    64) install_dir=installed ;;
    32) install_dir=installed32 ;;
    *) echo "Invalid Boost address model: $address_model" >&2; exit 1 ;;
esac

if [[ ! "$version" =~ ^[0-9]+(\.[0-9]+)*$ ]]; then
    echo "Invalid boost version: $version" >&2
    exit 1
fi

cd "$mydir"

# TODO: add progress bar, -q is quite, if removing it the progress bar is in
# multiple lines
echo "Downloading boost"
underscore_version=${version//./_}
wget -q --tries=3 "https://archives.boost.io/release/$version/source/boost_$underscore_version.tar.gz" -O "boost-$version.tar.gz"

echo "Unzipping boost"
temp_extract_dir=$(mktemp -d "./boost-$version.extract.XXXXXX")
staged_boost_dir="./boost-$version.new"
trap 'rm -rf -- "$temp_extract_dir"' EXIT
tar xzvf "boost-$version.tar.gz" -C "$temp_extract_dir" >> /dev/null

if [[ ! -d "$temp_extract_dir/boost_$underscore_version" ]]; then
    echo "Expected extracted directory boost_$underscore_version was not found" >&2
    exit 1
fi

rm -rf -- "$staged_boost_dir"
mv "$temp_extract_dir/boost_$underscore_version" "$staged_boost_dir"
rm -rf -- "boost-$version"
mv "$staged_boost_dir" "boost-$version"
trap - EXIT
rm -rf -- "$temp_extract_dir"

echo "Installing boost"
cd "boost-$version";
if [[ $OSTYPE == 'darwin'* ]]; then
    export SDKROOT=$(xcrun --sdk macosx --show-sdk-path)
fi
mkdir "$install_dir"
./bootstrap.sh --prefix="$(pwd)/$install_dir" --with-libraries=program_options,timer,chrono,thread,regex >> /dev/null
./b2 cxxflags="-fPIC -std=c++11" link=static install address-model="$address_model" >> /dev/null

cd "$curdir"
