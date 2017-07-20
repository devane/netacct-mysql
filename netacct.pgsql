/*
  Create PostgreSQL database for netacct

  by Vladislav Tsanev <tvlado@fibank.bg>
*/

CREATE SEQUENCE traffic_id_seq START 1;

CREATE TABLE rrd (
  ip varchar(15) NOT NULL default '',
  input bigint NOT NULL default '0',
  output bigint NOT NULL default '0',
  peer_input bigint NOT NULL default '0',
  peer_output bigint NOT NULL default '0',
  direct_input bigint NOT NULL default '0',
  direct_output bigint NOT NULL default '0',
  local_input bigint NOT NULL default '0',
  local_output bigint NOT NULL default '0'
);

CREATE TABLE traffic (
  id bigint DEFAULT nextval('traffic_id_seq') NOT NULL,
  ip varchar(15) NOT NULL default '',
  time timestamp NOT NULL,
  input bigint NOT NULL default '0',
  output bigint NOT NULL default '0',
  peer_input bigint NOT NULL default '0',
  peer_output bigint NOT NULL default '0',
  direct_input bigint NOT NULL default '0',
  direct_output bigint NOT NULL default '0',
  local_input bigint NOT NULL default '0',
  local_output bigint NOT NULL default '0'
);

CREATE INDEX rrd_ip_index ON rrd (ip);
CREATE INDEX traffic_id_index ON traffic (id);
CREATE INDEX traffic_ip_index ON traffic (ip);
CREATE INDEX traffic_time_index ON traffic (time);
CREATE INDEX traffic_ip_time_index ON traffic (ip,time);
