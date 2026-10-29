"""Pre-Build-Skript: macht VitoWiFi 1.x mit aktuellem GCC (espressif8266 >= 3.x) kompilierbar.

DPTypes.hpp überschreibt `virtual size_t getLength()` mit `const size_t getLength()`.
Neuere GCC-Versionen werten den abweichenden Rückgabetyp als Fehler.
Das Skript entfernt das überflüssige `const` in der installierten Bibliothek (idempotent).
"""
import os

Import("env")  # noqa: F821 (von PlatformIO bereitgestellt)

libdeps = os.path.join(env.subst("$PROJECT_LIBDEPS_DIR"), env.subst("$PIOENV"))  # noqa: F821
target = os.path.join(libdeps, "VitoWiFi", "src", "DPTypes.hpp")

if os.path.isfile(target):
    with open(target, encoding="utf-8") as f:
        code = f.read()
    patched = code.replace("const size_t getLength() const", "size_t getLength() const")
    if patched != code:
        with open(target, "w", encoding="utf-8") as f:
            f.write(patched)
        print("patch_vitowifi: DPTypes.hpp angepasst")
