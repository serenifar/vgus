sudo kill  `ps -e | grep temp_serv | awk '{print $1}'` 
