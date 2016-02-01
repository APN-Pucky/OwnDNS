
MYSQL* init_dns_mysql()
{
   char *server = "localhost";
   char *user = "dns_mysql";
   char *password = "lqs_snd"; /* set me first */
   char *database = "dns";//also table dns
   MYSQL *conn;
   conn = mysql_init(NULL);
   if (!mysql_real_connect(conn, server,
         user, password, database, 0, NULL, 0)) {
      fatal((char *)mysql_error(conn));
   }
   return conn;
}

void createTable(MYSQL *conn)
{
   char query[66];
   strcpy(query, "CREATE TABLE dns (dns VARCHAR(20) NOT NULL, ip CHAR(15) NOT NULL);");
   if (mysql_query(conn, query)) {
        fatal((char *) mysql_error(conn));
   }
}

void getIp(MYSQL *conn,unsigned char *dnsname, unsigned char *phpip,int *lock)
{
   int err = 1;
   char format[35+1];
   char query[35+strlen(dnsname)+1];
   MYSQL_RES *res;
   MYSQL_ROW row;
   strcpy(format, "SELECT ip FROM dns WHERE dns = '%s';");
   sprintf(query,format,dnsname);
   /* send SQL query */
   while(err == 1)
   {
    	err = 0;
   	while(*lock==1){usleep(10);};
   	*lock = 1;
  	if (mysql_query(conn, query)) 
	{
	   //printf("ERR\n");
	   err =1;
  	}
   }
   res = mysql_store_result(conn);
   //printf("Length: %i\n",(int)mysql_fetch_lengths(res)[0]);
   //row = (MYSQL_ROW *)malloc(mysql_fetch_lengths(res)[0]);
   //printf("POST MALLOC\n");
   //if(row == NULL)fatal("in malloc");
   //*row = mysql_fetch_row(res);
   row = mysql_fetch_row(res);
   if(row == NULL || row[0]== NULL || ((unsigned char *)row[0])==NULL || strlen((unsigned char *)row[0])<7)
   {
	sprintf(phpip,"%s",dnsname);
   }
   else
   {
  	sprintf(phpip,"%s",(unsigned char *)row[0]);
   }
   mysql_free_result(res);
   *lock = 0;
   return;
}

void setIp(MYSQL *conn,unsigned char *dnsname, unsigned char *ip, int *lock)
{
   int err = 1;
   char format[43+1];
   char query[43+strlen(dnsname)+strlen(ip)+1];
   strcpy(format, "INSERT INTO dns (dns,ip) VALUES('%s' ,'%s');");
   sprintf(query,format,dnsname, ip);
   while(err == 1)
   {
    	err = 0;
   	while(*lock==1){usleep(10);}
   	*lock = 1;
   	if (mysql_query(conn, query)) {
	   //printf("ERR2");
           err =1;
       	}
   }
   *lock = 0;

}

void close_dns_mysql(MYSQL *conn)
{
   mysql_close(conn);
}
