{
  description = "Horpkg - OpenHarmony HAP Installer for NixOS ARM";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs = { self, nixpkgs }:
    let
      # 支持 aarch64-linux (你的 ARM 系统) 和 x86_64-linux
      supportedSystems = [ "aarch64-linux" "x86_64-linux" ];
      forAllSystems = nixpkgs.lib.genAttrs supportedSystems;
      pkgsFor = system: import nixpkgs { inherit system; };
    in
    {
      # 默认包定义
      packages = forAllSystems (system: {
        default = (pkgsFor system).callPackage ./default.nix {};
      });

      # 开发环境定义 (nix develop)
      devShells = forAllSystems (system: {
        default = import ./shell.nix { pkgs = pkgsFor system; };
      });
    };
}
