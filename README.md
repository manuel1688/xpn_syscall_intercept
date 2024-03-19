#El directorio xpn_syscall_intercept contiene la nueva libreria de intercepcion
#que debe ser compilada con syscall_intercept.git
1. Hace pull del xpn_syscall_intercept: https://github.com/manuel1688/xpn_syscall_intercept.git

# Dentro de directiro xpn_syscall_intercept, syscall_intercept es la libreria
# de intel que usaremos para compilar la nueva libreria de xpn
2. git clone https://github.com/pmem/syscall_intercept.git
3. sudo apt install clang
4. sudo apt-get install pkg-config libcapstone-dev
5. cmake syscall_intercept -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang
6. make
7. sudo make install

#compilar la nueva libreria
# makefile_scripts.sh se encarga de reemplazar el makefile de syscall_intercept por
# la nueva libreria de intercepcion
#TODO se debe remover todos los archivos de syscall_intercept.git ya que solo se usa para compilar
#la nueva liberia
8. Ejecutar el script makefile_scripts.sh
9. make

#xpn/test/integrity
10. hace clone de test_xpn_syscall : https://github.com/manuel1688/test_xpn_syscall.git
11. make
#para ejecutar las pruebas de la nueva liberia se usa el scrit:
12. run_create_test.sh
