MODO DE MANTENIMIENTO

Esta es una característica agregada en la versión 0.5 tanto del servidor
SAGS como del cliente. Su función principal es impedir que un servidor
se reinicie si es que está en mantenimiento, es decir que en este modo
no se considerará la variable Respawn del archivo de configuración del
servidor. Esto permite a un usuario administrador hacerle mantenimiento
a un servidor de juego (ej. editar los archivos de configuración del
juego, agregar mapas, etc.) luego de detenerlo.

La utilización de este modo debe activarse desde el Cliente SAGS, por lo
que se puede conseguir mayor información de su uso en la documentación
correspondiente al cliente.

La incorporación de este modo también introdujo más información a los
procesos. Esta información comprende si está activado o no el modo de
mantenimiento (MaintenanceMode), la fecha del último mantenimiento hecho
(MaintenanceLastStart), su duración (MaintenanceDuration) y el número de
veces que se ha hecho mantenimiento (MaintenanceCount).

