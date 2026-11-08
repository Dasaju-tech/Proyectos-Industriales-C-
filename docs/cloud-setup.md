# Entorno de desarrollo reutilizable

## Preparación

Usar el checkout existente de `Dasaju-tech/Proyectos-Industriales-C-` bajo
`/workspace/Proyectos-Industriales-C-`. Cada tarea de nube ya está aislada:
no crear un worktree salvo petición explícita del usuario.

Requisitos: Python 3.8+, `g++` con C++03 y sanitizadores (para la comprobación
de memoria). La imagen usada ya trae Python 3.12.14 y GCC 14.2. No se necesitan
instalaciones adicionales, credenciales ni servicios.

Instrucciones de preparación desde el checkout:

```sh
python3 tools/dev.py test
```

Esto comprueba la generación, construye el ejecutable y ejecuta las pruebas.
Los resultados se escriben en `build/`, excluido de Git. No hay servidor que
deba permanecer en ejecución ni proceso que restaurar.

## Inicio de una tarea

1. Entrar en el checkout existente e inspeccionar `git status` y las instrucciones
   locales. Conservar los cambios del usuario.
2. Ejecutar `python3 tools/dev.py test` para construir y verificar el estado
   actual. Si cambia el modelo, regenerarlo con
   `python3 tools/generate_model.py` antes de validar.
3. Para estudiar la respuesta, ejecutar
   `./build/motor --simulate 20 1 10 100 1000`. Debe salir con código cero,
   encabezado CSV, estado inicial y 20 pasos; en el primer paso `Vo=0.1 V`,
   `Vel=10 rpm` y `Pos=60 grados`.
4. Tras cambios de C++, ejecutar `python3 tools/dev.py test --sanitize`.

La validación obligatoria de desarrollo portable es la generación actualizada,
compilación y pruebas funcionales; los sanitizadores comprueban además el uso de
memoria y comportamiento indefinido. Para declarar validado el destino VS2008,
se requiere por separado Windows con VS2008 C++ y ejecutar
`tools\build_vs2008.cmd`. La imagen Linux no dispone de ese compilador ni de su
SDK; tampoco de `sudo` o del instalador privilegiado de paquetes.

## Estado de configuración

Este documento y los scripts están versionados para poder repetir el flujo
sin depender de la configuración externa. Guardar un borrador de entorno no
ejecuta sus instrucciones ni lo publica. La publicación y la restauración en
una tarea nueva necesitan validarse de forma independiente.

Durante esta preparación, la herramienta de plataforma no pudo verificar el
borrador y el intento de guardar `install_script` y `start_skill` devolvió un
error sin confirmar persistencia. Las instrucciones quedan guardadas en este
repositorio; no se afirma que el borrador externo se haya guardado o publicado.
