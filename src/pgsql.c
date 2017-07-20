 /*
  PostgreSQL driver for netacct 
  bug & comments to Vladislav Tsanev <tvlado@fibank.bg>
  */

#include "netacct.h"

#ifdef HAVE_PGSQL
#include <libpq-fe.h>

static char rcsid[] = "$Id: pgsql.c,v 1.10 2005/03/31 11:31:00 vlado Exp $";

#undef _DEBUG_
#define MAXCONNINFO 180

#define TTABLE "traffic"

int write_pgsql(struct HOST_DATA* tmpData, PGconn* conn) {
    PGresult   *res;
    
    int nFields;
    char query[8192];
    char rrd_query[8192];
    char TIME_MASK[] = "to_timestamp(to_char(now(), 'DD-Mon-YYYY HH24'), 'DD-Mon-YYYY HH24')";    
    char spyip[16];
    unsigned long long rows;
    unsigned long traffic[8];
    unsigned long input, output, peer_input, peer_output, id;
    unsigned long direct_input, direct_output, local_input, local_output;

    
    input=(tmpData->nPeerFlag==0)?tmpData->nInTrafic:0;
    output=(tmpData->nPeerFlag==0)?tmpData->nOutTrafic:0;
    peer_input=(tmpData->nPeerFlag==1)?tmpData->nInTrafic:0;
    peer_output=(tmpData->nPeerFlag==1)?tmpData->nOutTrafic:0;
    direct_input=(tmpData->nPeerFlag==2)?tmpData->nInTrafic:0;
    direct_output=(tmpData->nPeerFlag==2)?tmpData->nOutTrafic:0;
    local_input=(tmpData->nPeerFlag==3)?tmpData->nInTrafic:0;
    local_output=(tmpData->nPeerFlag==3)?tmpData->nOutTrafic:0;
    strncpy(spyip,intoa(tmpData->ipAddress),16);
    
/*
 *    RRD QUERY
 */
    sprintf(rrd_query, "SELECT input,output,peer_input,peer_output,\
direct_input,direct_output,local_input,local_output FROM rrd WHERE ip='%s'", spyip);

#ifdef _DEBUG_
    printf("%s\n", rrd_query);    
#endif
 
    res = PQexec(conn, rrd_query);
    if (!res || PQresultStatus(res) != PGRES_TUPLES_OK) {
	syslog(LOG_ERR, "PostgreSQL RRD QUERY FAILED:%s", PQerrorMessage(conn));
    	PQclear(res);
	return 1;
    }
    rows = PQntuples(res);
    nFields = PQnfields(res);

    if(rows>1) {
	/* 
	 * Returned rows never has been great than 1
	 */
	syslog(LOG_INFO, "PostgreSQL: %s returned %d rows", rrd_query, rows);
	PQclear(res);
	return 1;
    } else if(rows == 1) {
	traffic[0]=atol(PQgetvalue(res, 0, 0));
	traffic[1]=atol(PQgetvalue(res, 0, 1));
	traffic[2]=atol(PQgetvalue(res, 0, 2));
	traffic[3]=atol(PQgetvalue(res, 0, 3));
	traffic[4]=atol(PQgetvalue(res, 0, 4));
	traffic[5]=atol(PQgetvalue(res, 0, 5));
	traffic[6]=atol(PQgetvalue(res, 0, 6));
	traffic[7]=atol(PQgetvalue(res, 0, 7));
	 sprintf(rrd_query,"UPDATE rrd SET input=%lu,output=%lu,\
peer_input=%lu,peer_output=%lu,direct_input=%lu,direct_output=%lu,\
local_input=%lu,local_output=%lu WHERE ip='%s'",
            (tmpData->nPeerFlag==0)?tmpData->nInTrafic:traffic[0],\
            (tmpData->nPeerFlag==0)?tmpData->nOutTrafic:traffic[1],\
            (tmpData->nPeerFlag==1)?tmpData->nInTrafic:traffic[2],\
            (tmpData->nPeerFlag==1)?tmpData->nOutTrafic:traffic[3],\
            (tmpData->nPeerFlag==2)?tmpData->nInTrafic:traffic[4],\
            (tmpData->nPeerFlag==2)?tmpData->nOutTrafic:traffic[5],\
            (tmpData->nPeerFlag==3)?tmpData->nInTrafic:traffic[6],\
            (tmpData->nPeerFlag==3)?tmpData->nOutTrafic:traffic[7], spyip );
    } else {
        sprintf(rrd_query,"INSERT INTO rrd (ip,input,output,peer_input,\
peer_output,direct_input,direct_output,local_input,local_output)\
 VALUES ( '%s','%lu','%lu','%lu','%lu','%lu','%lu','%lu','%lu')",\
            spyip,(tmpData->nPeerFlag==0)?tmpData->nInTrafic:0,\
            (tmpData->nPeerFlag==0)?tmpData->nOutTrafic:0,\
            (tmpData->nPeerFlag==1)?tmpData->nInTrafic:0,\
            (tmpData->nPeerFlag==1)?tmpData->nOutTrafic:0,\
            (tmpData->nPeerFlag==2)?tmpData->nInTrafic:0,\
            (tmpData->nPeerFlag==2)?tmpData->nOutTrafic:0,\
            (tmpData->nPeerFlag==3)?tmpData->nInTrafic:0,\
            (tmpData->nPeerFlag==3)?tmpData->nOutTrafic:0);
    }
    PQclear(res);
    
    res = PQexec(conn, rrd_query);
    if (!res || PQresultStatus(res) != PGRES_COMMAND_OK) {
	syslog(LOG_ERR, "PostgreSQL RRDQUERY: %s FAILED: %s", rrd_query, PQerrorMessage(conn));
    	PQclear(res);
	return 1;
    }
    PQclear(res);

#ifdef _DEBUG_
    printf("%s\n", rrd_query);
#endif

/*
 *    TRAFFIC QUERY
 */ 
     sprintf(query, "SELECT id FROM %s WHERE ip = '%s' and time = %s", \
	TTABLE, spyip, TIME_MASK);

#ifdef _DEBUG_     
     printf("%s\n", query);
#endif

    res = PQexec(conn, query);
    if (!res || PQresultStatus(res) != PGRES_TUPLES_OK) {
	syslog(LOG_ERR, "PostgreSQL TRAFFIC QUERY FAILED:%s", PQerrorMessage(conn));
    	PQclear(res);
	return 1;
    }    
    rows = PQntuples(res);
    
    if(rows>1) {
       /*
        * Returned rows never has been great than 1
        */
	syslog(LOG_INFO, "PostgreSQL: %s returned %d rows", rrd_query, rows);
	PQclear(res);
	return 1;
    } else if(rows == 1) {
	id=atol(PQgetvalue(res,0,0));	
	sprintf(query, "UPDATE %s SET input=input + %lu,output=output + %lu,\
peer_input=peer_input + %lu,peer_output=peer_output + %lu,\
direct_input=direct_input + %lu,direct_output=direct_output + %lu,\
local_input=local_input + %lu,local_output=local_output + %lu WHERE id = %lu",\
TTABLE, input, output, peer_input, peer_output, direct_input,\
direct_output, local_input, local_output, id);
    } else {
	sprintf(query, "INSERT INTO %s (ip,time,input,output,peer_input,peer_output,\
direct_input,direct_output,local_input,local_output) VALUES('%s',%s,'%lu','%lu',\
'%lu','%lu','%lu','%lu','%lu','%lu')", TTABLE, spyip, TIME_MASK, input, \
output, peer_input, peer_output, direct_input, direct_output, local_input, \
local_output);
    }
    PQclear(res);
    
    res = PQexec(conn, query);
    if (!res || PQresultStatus(res) != PGRES_COMMAND_OK) {
	syslog(LOG_ERR, "PostgreSQL QUERY: %s FAILED: %s", query, PQerrorMessage(conn));
    	PQclear(res);
	return 1;
    }
    PQclear(res);
#ifdef _DEBUG_        
    printf("%s\n", query);
#endif  
    return 0;
}

/* return values: 0 suceed, !=0 failed */
int do_write_list_pgsql(void) {
	PGconn     *conn;
	int	   res;
	struct HOST_DATA* tmpData;
        char conninfo[MAXCONNINFO];
	
	tmpData = (struct HOST_DATA*) GetFirstHost();
	
#ifdef _DEBUG_
	syslog(LOG_INFO, "do_write_list_pgsql called");	
#endif	

	if(tmpData == 0) return 0;	

        sprintf(conninfo, "hostaddr=%s port=%d dbname=%s user=%s password=%s", \
         cfg->pgsql_host, cfg->pgsql_port, cfg->pgsql_database, cfg->pgsql_user,
\
         cfg->pgsql_password);

        conn = PQconnectdb(conninfo);
        if (PQstatus(conn) != CONNECTION_OK) {
	    syslog(LOG_INFO,"PostgreSQL Error: %s",  PQerrorMessage(conn));
	    return 1;
        }
	
	if((tmpData->written == 0) && ((tmpData->nInTrafic != 0) || (tmpData->nOutTrafic != 0))) {
		res = write_pgsql(tmpData,conn);
		if(res != 0) {
		 /* error in writting to DB */
		  PQfinish(conn);
		  return 1;
		} else {
		  tmpData->written = 1;
		  tmpData->nInTrafic = 0;
		  tmpData->nOutTrafic = 0;
		}
	}

	while(tmpData = (struct HOST_DATA*) GetNextHostData()) {	    
		 if((tmpData->written == 0) && ((tmpData->nInTrafic != 0) || (tmpData->nOutTrafic != 0))) {
		  res = write_pgsql(tmpData,conn);
		  if(res != 0) {
		    /* error in writting to DB */
		    PQfinish(conn);
    		    return 1;
		   } else {
		    tmpData->written = 1;
		    tmpData->nInTrafic = 0;
		    tmpData->nOutTrafic = 0;
		  }
		 }
	}
	/* normal exit */
        PQfinish(conn);
	return 0;
}

void* write_main_pgsql(void) {	
    WRITE_LOOP(do_write_list_pgsql());
}

#endif /*HAVE PG_SQL*/ 
