{ lib
, stdenv
, src
, cmake
, ninja
, qtbase
, wrapQtAppsHook
, libneurosuite
}:

stdenv.mkDerivation {
  pname = "neuroscope";
  version = "3.0.0";
  inherit src;

  nativeBuildInputs = [ cmake ninja wrapQtAppsHook ];
  buildInputs = [ qtbase libneurosuite ];

  doCheck = true;

  meta = {
    description = "Viewer for neurophysiological and behavioral data";
    homepage = "https://neurosuite.github.io";
    license = lib.licenses.gpl3Plus;
    mainProgram = "neuroscope";
    platforms = lib.platforms.unix;
  };
}
