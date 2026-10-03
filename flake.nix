{
  description = "NeuroScope: viewer for neurophysiological and behavioral data";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    libneurosuite = {
      url = "github:neurosuite/libneurosuite";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs = { self, nixpkgs, libneurosuite }:
    let
      systems = [ "x86_64-linux" "aarch64-linux" "aarch64-darwin" ];
      forAllSystems = f: nixpkgs.lib.genAttrs systems (system: f system nixpkgs.legacyPackages.${system});
    in
    {
      packages = forAllSystems (system: pkgs: rec {
        neuroscope = pkgs.qt6Packages.callPackage ./nix/package.nix {
          src = self;
          libneurosuite = libneurosuite.packages.${system}.libneurosuite;
        };
        default = neuroscope;
      });

      devShells = forAllSystems (system: pkgs: {
        default = pkgs.mkShell {
          inputsFrom = [ self.packages.${system}.default ];
          packages = [ pkgs.clang-tools ];
        };
      });
    };
}
