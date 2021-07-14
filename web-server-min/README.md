# min-web-server

  CGI, directory listings, serve static files

  update: OP_ENERGY calculator -- idA=blockA&idB=blockB
   where blockB > blockA and both are within range (649000)   e.g.

    curl 'http://dev.bergain.biz:8111//server/cgi_bin/calcops_28.cgi?idA=647136&idB=649152'


  update: OP_ENERGY csv generator   e.g.

    http://dev.bergain.biz:8113/server/cgi_bin/calcops_283.cgi?idBeg=200200&span=2016&idEnd=246000





Python MIN Server Features:

  0. Basic CGI support
  1. Single connection and parallel thread mode
  2. HTTP GET requests with query and header parsing
  3. Automatic directory listing 
  4. Static file transport (download files)
