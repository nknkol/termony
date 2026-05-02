{ pkgs ? import <nixpkgs> {} }:

pkgs.stdenv.mkDerivation rec {
  pname = "horpkg";
  version = "1.0-nixos";

  src = ./.;

  nativeBuildInputs = with pkgs; [
    pkg-config
    patchelf
    gcc
  ];

  buildInputs = with pkgs; [
    curl
    yyjson
    libzip
    zlib
  ];

  buildPhase = ''
    runHook preBuild
    SRCS=$(ls src/*.c | grep -v "core_hnp_installer.c")
    mkdir -p dist/bin dist/etc
    
    gcc -std=c11 -O2 -Wno-unused-parameter \
        $SRCS \
        -o dist/bin/horpkg \
        $(pkg-config --cflags --libs libcurl yyjson libzip) \
        -DHORPKG_RUNTIME_PIN=\"314159\" \
        -I./src
        
    cp src/cacert.pem dist/etc/
    runHook postBuild
  '';

  installPhase = ''
    runHook preInstall
    mkdir -p $out/bin $out/etc
    cp dist/bin/horpkg $out/bin/
    cp dist/etc/cacert.pem $out/etc/
    
    patchelf --set-interpreter "$(cat $NIX_CC/nix-support/dynamic-linker)" $out/bin/horpkg
    runHook postInstall
  '';
}
