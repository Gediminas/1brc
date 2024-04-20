#! /usr/bin/env nix-shell

let
  unstableTarball = fetchTarball "https://github.com/NixOS/nixpkgs/archive/nixos-unstable.tar.gz";
  # pkgs = import <nixpkgs> {}; 
  unstable = import unstableTarball {};
in
with import <nixpkgs> {}; {
  qpidEnv = stdenvNoCC.mkDerivation {
    name = "1-brc";
    nativeBuildInputs = [ 
      unstable.gcc13
      unstable.clang-tools_18
      unstable.clang_18
    ];
    buildInputs = [
      unstable.liburing
      unstable.liburing.dev
      unstable.gnumake
      unstable.entr
      unstable.hyperfine
      unstable.linuxKernel.packages.linux_hardened.perf
      # unstable.jdk
      # uftrace
      # hotspot
      # kcachegrind
      # graphviz
      # fio
    ];
    shellHook = ''
      up_dir=$(dirname "$(pwd)")

      export FULL="$up_dir/_data/full.txt"
      export SMALL="$up_dir/_data/small.txt"
      export TINY="$up_dir/_data/tiny.txt"
      
      echo "FULL:  $FULL"
      echo "SMALL: $SMALL"
      echo "TINY:  $TINY"
      
      # export CPATH="4x12lgfjm8768kg4si12c3wd34i14hg5-liburing-2.5-dev/include"
      # CPATH = lib.makeSearchPathOutput "dev" "include" buildInputs;

      echo ">>>>> $name"
      echo -n ">>>>> "; gcc --version | head -n1 | awk '{print $1, $3, $2}'
      echo -n ">>>>> "; clang --version | head -n1
      echo -n ">>>>> "; hyperfine --version
    '';
  };
}
