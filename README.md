#El directorio xpn_syscall_intercept contiene la nueva libreria de intercepcion
#La nueva libreria debe ser compilada con syscall_intercept.git

#En el path xpn/src de xpand
1. cd src/xpn/src/    
2. Hace clone del xpn_syscall_intercept: git clone https://github.com/manuel1688/xpn_syscall_intercept.git
3. cd xpn_syscall_intercept

# Dentro del path xpn_syscall_intercept,se debe hacer clone de la libreria de Intel
# para compilar la nueva libreria de xpn
2. git clone https://github.com/pmem/syscall_intercept.git
3. sudo apt install clang 
4. sudo apt-get install pkg-config libcapstone-dev
5. cmake syscall_intercept -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang
6. make
7. sudo make install

#compilar la nueva libreria
# makefile_scripts.sh se encarga de reemplazar el makefile de Intel por el
# Makefile para la nueva libreria de intercepcion
#TODO se debe remover todos los archivos de syscall_intercept.git ya que solo se usa para compilar OPCIONAL
#la nueva liberia
#TODO mover el xpn_syscall_intercept_create.h a la carpeta include
#TODO revisar los includes de xpn_syscall_intercept_create.h a ver si todo son necesarios para la 
#nueva libreria
8. bash makefile_scripts.sh
9. cd ../../test/integrity/

#xpn/test/integrity
9. hace clone de test_xpn_syscall : git clone https://github.com/manuel1688/test_xpn_syscall.git
10. cd test_xpn_syscall/
10. make
#para ejecutar las pruebas de la nueva liberia se usa el scrit:
11. run_create_test.sh




test b8f4525a22d367f3b7abfc16f250c69ec953e159
git rev-parse HEAD

git revert b8f4525a22d367f3b7abfc16f250c69ec953e159

code dc39856f55e1d2190464ede5dc04057ecff4bead

git revert dc39856f55e1d2190464ede5dc04057ecff4bead


LD_PRELOAD=../../../src/xpn_syscall_intercept/xpn_syscall_intercept_create.so  XPN_CONF=./xpn.conf ./prueba_create /tmp/expand/P1/demo.txt  8
LD_PRELOAD=../../../src/xpn_syscall_intercept/xpn_syscall_intercept_create.so:$LD_PRELOAD  XPN_CONF=./xpn.conf ./open_write_close /tmp/expand/P1/demo.txt  3

LD_PRELOAD="../../../src/xpn_syscall_intercept/xpn_syscall_intercept_create.so:../../../../mxml/libmxml.so.1:$LD_PRELOAD"  XPN_CONF="./xpn.conf" ./prueba_create /tmp/expand/P1/demo.txt  3

LD_PRELOAD="../../../src/xpn_syscall_intercept/xpn_syscall_intercept_create.so:$LD_PRELOAD"  XPN_CONF="./xpn.conf" ./prueba_create /tmp/expand/P1/demo.txt 3

