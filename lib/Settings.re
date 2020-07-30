/* Type scaffolding */
[@deriving (show({with_path: false}), eq)]
type execution_mode =
  | Standalone
  | Coordinator
  | Worker;

[@deriving (show({with_path: false}), eq)]
type queue_connector =
  | Disabled
  | AMQP
  | Redis;

[@deriving (show({with_path: false}), eq)]
type amqp_consumer = {
  host: string,
  port: int,
  ssl: bool,
  username: string,
  [@opaque]
  password: string,
  vhost: string,
  exchange: string,
  queue: string,
  topic: string,
};

[@deriving (show({with_path: false}), eq)]
type redis_consumer = {
  host: string,
  port: int,
  ssl: bool,
  username: option(string),
  [@opaque]
  password: option(string),
  channel: string,
};

[@deriving (show({with_path: false}), eq)]
type consumer = {
  connector: queue_connector,
  amqp: amqp_consumer,
  redis: redis_consumer,
};

[@deriving (show({with_path: false}), eq)]
type amqp_forwarder = {
  host: string,
  port: int,
  ssl: bool,
  username: string,
  [@opaque]
  password: string,
  vhost: string,
  exchange: string,
  topic: string,
};

[@deriving (show({with_path: false}), eq)]
type redis_forwarder = {
  host: string,
  port: int,
  ssl: bool,
  username: option(string),
  [@opaque]
  password: option(string),
  channel: string,
};

[@deriving (show({with_path: false}), eq)]
type forwarder = {
  connector: queue_connector,
  amqp: amqp_forwarder,
  redis: redis_forwarder,
};

[@deriving (show({with_path: false}), eq)]
type graph_api = {
  [@printer (fmt, a) => fprintf(fmt, "%s", Unix.string_of_inet_addr(a))]
  [@equal (a, b) => Unix.string_of_inet_addr(a) == Unix.string_of_inet_addr(b)]
  listen_address: Unix.inet_addr,
  listen_port: int,
  dot_command: string,
};

[@deriving (show({with_path: false}), eq)]
type work_api = {
  [@printer (fmt, a) => fprintf(fmt, "%s", Unix.string_of_inet_addr(a))]
  [@equal (a, b) => Unix.string_of_inet_addr(a) == Unix.string_of_inet_addr(b)]
  listen_address: Unix.inet_addr,
  listen_port: int,
};

[@deriving (show({with_path: false}), eq)]
type db_connector =
  | SQLite
  | Postgres
  | ElasticSearch;

[@deriving (show({with_path: false}), eq)]
type graph_db_sqlite = {path: string};

[@deriving (show({with_path: false}), eq)]
type graph_db_postgres = {
  host: string,
  port: int,
  ssl: bool,
  username: string,
  [@opaque]
  password: string,
  dbname: string,
};

[@deriving (show({with_path: false}), eq)]
type graph_db = {
  connector: db_connector,
  sqlite: graph_db_sqlite,
  postgres: graph_db_postgres,
};

[@deriving (show({with_path: false}), eq)]
type timeseries_db_sqlite = {path: string};

[@deriving (show({with_path: false}), eq)]
type timeseries_db_postgres = {
  host: string,
  port: int,
  ssl: bool,
  username: string,
  [@opaque]
  password: string,
  dbname: string,
};

[@deriving (show({with_path: false}), eq)]
type timeseries_db_elasticsearch = {
  host: string,
  port: int,
  ssl: bool,
  username: option(string),
  [@opaque]
  password: option(string),
  template_prefix: string,
};

[@deriving (show({with_path: false}), eq)]
type timeseries_db = {
  connector: db_connector,
  sqlite: timeseries_db_sqlite,
  postgres: timeseries_db_postgres,
  elasticsearch: timeseries_db_elasticsearch,
};

[@deriving (show({with_path: false}), eq)]
type t = {
  [@printer (fmt, level) => fprintf(fmt, "%s", Dolog.Log.string_of_level(level))]
  [@equal (a, b) => a === b]
  log_level: Dolog.Log.log_level,
  execution_mode,
  consumer,
  forwarder,
  graph_api,
  work_api,
  graph_db,
  timeseries_db,
};

/* Default settings */
let defaults: t = {
  log_level: Dolog.Log.INFO,
  execution_mode: Standalone,
  consumer: {
    connector: Disabled,
    amqp: {
      host: "localhost",
      port: 5672,
      ssl: false,
      username: "guest",
      password: "guest",
      vhost: "/",
      exchange: "data.ingest",
      queue: "data.ingest-queue",
      topic: "data.ingest-frames",
    },
    redis: {
      host: "localhost",
      port: 6379,
      ssl: false,
      username: None,
      password: None,
      channel: "data.ingest",
    },
  },
  forwarder: {
    connector: Disabled,
    amqp: {
      host: "localhost",
      port: 5672,
      ssl: false,
      username: "guest",
      password: "guest",
      vhost: "/",
      exchange: "data.digest",
      topic: "data.digest-frames",
    },
    redis: {
      host: "localhost",
      port: 6379,
      ssl: false,
      username: None,
      password: None,
      channel: "data.digest",
    },
  },
  graph_api: {
    listen_address: Unix.inet_addr_of_string("127.0.0.1"),
    listen_port: 8000,
    dot_command: "/usr/bin/env dot",
  },
  work_api: {
    listen_address: Unix.inet_addr_of_string("127.0.0.1"),
    listen_port: 8001,
  },
  graph_db: {
    connector: SQLite,
    sqlite: {
      path: "./graph.db",
    },
    postgres: {
      host: "localhost",
      port: 5432,
      ssl: false,
      username: "grace",
      password: "grace",
      dbname: "graph",
    },
  },
  timeseries_db: {
    connector: SQLite,
    sqlite: {
      path: "./timeseries.db",
    },
    postgres: {
      host: "localhost",
      port: 5432,
      ssl: false,
      username: "grace",
      password: "grace",
      dbname: "timeseries",
    },
    elasticsearch: {
      host: "localhost",
      port: 9200,
      ssl: false,
      username: None,
      password: None,
      template_prefix: "timeseries",
    },
  },
};

/**
 * Initialize from combined sources in the following order of cascading attempts:
 * 1. environment variables
 * 2. configuration file, if given
 * 3. defaults
 */
let initialize: (string => option(string), option(string)) => t =
  (env, config_file_path) => {
    open SettingsParser;
    let from_env = env_find(env);
    let from_file: string => option(Yaml.value) =
      switch (config_file_path) {
      | Some(config) => yaml_find(Yaml_unix.of_file_exn(Fpath.v(config)))
      | None => (_ => None)
      };
    {
      log_level:
        log_level_p(from_env("LOG_LEVEL"), from_file("logLevel"), defaults.log_level),
      execution_mode:
        enum_p(
          [
            ("standalone", Standalone),
            ("coordinator", Coordinator),
            ("worker", Worker),
          ],
          from_env("EXECUTION_MODE"),
          from_file("executionMode"),
          defaults.execution_mode,
        ),
      consumer: {
        connector:
          enum_p(
            [("disabled", Disabled), ("amqp", AMQP), ("redis", Redis)],
            from_env("CONSUMER_CONNECTOR"),
            from_file("consumer.connector"),
            defaults.consumer.connector,
          ),
        amqp: {
          host:
            non_empty_string_p(
              from_env("CONSUMER_AMQP_HOST"),
              from_file("consumer.amqp.host"),
              defaults.consumer.amqp.host,
            ),
          port:
            unix_port_p(
              from_env("CONSUMER_AMQP_PORT"),
              from_file("consumer.amqp.port"),
              defaults.consumer.amqp.port,
            ),
          ssl:
            boolean_p(
              from_env("CONSUMER_AMQP_SSL"),
              from_file("consumer.amqp.ssl"),
              defaults.consumer.amqp.ssl,
            ),
          username:
            non_empty_string_p(
              from_env("CONSUMER_AMQP_USERNAME"),
              from_file("consumer.amqp.username"),
              defaults.consumer.amqp.username,
            ),
          password:
            non_empty_string_p(
              from_env("CONSUMER_AMQP_PASSWORD"),
              from_file("consumer.amqp.password"),
              defaults.consumer.amqp.password,
            ),
          vhost:
            string_p(
              from_env("CONSUMER_AMQP_VHOST"),
              from_file("consumer.amqp.vhost"),
              defaults.consumer.amqp.vhost,
            ),
          exchange:
            non_empty_string_p(
              from_env("CONSUMER_AMQP_EXCHANGE"),
              from_file("consumer.amqp.exchange"),
              defaults.consumer.amqp.exchange,
            ),
          queue:
            non_empty_string_p(
              from_env("CONSUMER_AMQP_QUEUE"),
              from_file("consumer.amqp.queue"),
              defaults.consumer.amqp.queue,
            ),
          topic:
            non_empty_string_p(
              from_env("CONSUMER_AMQP_TOPIC"),
              from_file("consumer.amqp.topic"),
              defaults.consumer.amqp.topic,
            ),
        },
        redis: {
          host:
            non_empty_string_p(
              from_env("CONSUMER_REDIS_HOST"),
              from_file("consumer.redis.host"),
              defaults.consumer.redis.host,
            ),
          port:
            unix_port_p(
              from_env("CONSUMER_REDIS_PORT"),
              from_file("consumer.redis.port"),
              defaults.consumer.redis.port,
            ),
          ssl:
            boolean_p(
              from_env("CONSUMER_REDIS_SSL"),
              from_file("consumer.redis.ssl"),
              defaults.consumer.amqp.ssl,
            ),
          username:
            optional_string_p(
              from_env("CONSUMER_REDIS_USERNAME"),
              from_file("consumer.redis.username"),
              defaults.consumer.redis.username,
            ),
          password:
            optional_string_p(
              from_env("CONSUMER_REDIS_PASSWORD"),
              from_file("consumer.redis.password"),
              defaults.consumer.redis.password,
            ),
          channel:
            non_empty_string_p(
              from_env("CONSUMER_REDIS_CHANNEL"),
              from_file("consumer.redis.channel"),
              defaults.consumer.redis.channel,
            ),
        },
      },
      forwarder: {
        connector:
          enum_p(
            [("disabled", Disabled), ("amqp", AMQP), ("redis", Redis)],
            from_env("FORWARDER_CONNECTOR"),
            from_file("forwarder.connector"),
            defaults.forwarder.connector,
          ),
        amqp: {
          host:
            non_empty_string_p(
              from_env("FORWARDER_AMQP_HOST"),
              from_file("forwarder.amqp.host"),
              defaults.forwarder.amqp.host,
            ),
          port:
            unix_port_p(
              from_env("FORWARDER_AMQP_PORT"),
              from_file("forwarder.amqp.port"),
              defaults.forwarder.amqp.port,
            ),
          ssl:
            boolean_p(
              from_env("FORWARDER_AMQP_SSL"),
              from_file("forwarder.amqp.ssl"),
              defaults.forwarder.amqp.ssl,
            ),
          username:
            non_empty_string_p(
              from_env("FORWARDER_AMQP_USERNAME"),
              from_file("forwarder.amqp.username"),
              defaults.forwarder.amqp.username,
            ),
          password:
            non_empty_string_p(
              from_env("FORWARDER_AMQP_PASSWORD"),
              from_file("forwarder.amqp.password"),
              defaults.forwarder.amqp.password,
            ),
          vhost:
            string_p(
              from_env("FORWARDER_AMQP_VHOST"),
              from_file("forwarder.amqp.vhost"),
              defaults.forwarder.amqp.vhost,
            ),
          exchange:
            non_empty_string_p(
              from_env("FORWARDER_AMQP_EXCHANGE"),
              from_file("forwarder.amqp.exchange"),
              defaults.forwarder.amqp.exchange,
            ),
          topic:
            non_empty_string_p(
              from_env("FORWARDER_AMQP_TOPIC"),
              from_file("forwarder.amqp.topic"),
              defaults.forwarder.amqp.topic,
            ),
        },
        redis: {
          host:
            non_empty_string_p(
              from_env("FORWARDER_REDIS_HOST"),
              from_file("forwarder.redis.host"),
              defaults.forwarder.redis.host,
            ),
          port:
            unix_port_p(
              from_env("FORWARDER_REDIS_PORT"),
              from_file("forwarder.redis.port"),
              defaults.forwarder.redis.port,
            ),
          ssl:
            boolean_p(
              from_env("FORWARDER_REDIS_SSL"),
              from_file("forwarder.redis.ssl"),
              defaults.forwarder.amqp.ssl,
            ),
          username:
            optional_string_p(
              from_env("FORWARDER_REDIS_USERNAME"),
              from_file("forwarder.redis.username"),
              defaults.forwarder.redis.username,
            ),
          password:
            optional_string_p(
              from_env("FORWARDER_REDIS_PASSWORD"),
              from_file("forwarder.redis.password"),
              defaults.forwarder.redis.password,
            ),
          channel:
            non_empty_string_p(
              from_env("FORWARDER_REDIS_CHANNEL"),
              from_file("forwarder.redis.channel"),
              defaults.forwarder.redis.channel,
            ),
        },
      },
      graph_api: {
        listen_address:
          unix_address_p(
            from_env("GRAPH_API_LISTEN_ADDRESS"),
            from_file("graphAPI.listenAddress"),
            defaults.graph_api.listen_address,
          ),
        listen_port:
          unix_port_p(
            from_env("GRAPH_API_LISTEN_PORT"),
            from_file("graphAPI.listenPort"),
            defaults.graph_api.listen_port,
          ),
        dot_command:
          non_empty_string_p(
            from_env("GRAPH_API_DOT_COMMAND"),
            from_file("graphAPI.dotCommand"),
            defaults.graph_api.dot_command,
          ),
      },
      work_api: {
        listen_address:
          unix_address_p(
            from_env("WORK_API_LISTEN_ADDRESS"),
            from_file("workAPI.listenAddress"),
            defaults.work_api.listen_address,
          ),
        listen_port:
          unix_port_p(
            from_env("WORK_API_LISTEN_PORT"),
            from_file("workAPI.listenPort"),
            defaults.work_api.listen_port,
          ),
      },
      graph_db: {
        connector:
          enum_p(
            [("sqlite", SQLite), ("postgres", Postgres)],
            from_env("GRAPH_DB_CONNECTOR"),
            from_file("graphDB.connector"),
            defaults.graph_db.connector,
          ),
        sqlite: {
          path:
            non_empty_string_p(
              from_env("GRAPH_DB_SQLITE_PATH"),
              from_file("graphDB.sqlite.path"),
              defaults.graph_db.sqlite.path,
            ),
        },
        postgres: {
          host:
            non_empty_string_p(
              from_env("GRAPH_DB_POSTGRES_HOST"),
              from_file("graphDB.postgres.host"),
              defaults.graph_db.postgres.host,
            ),
          port:
            unix_port_p(
              from_env("GRAPH_DB_POSTGRES_PORT"),
              from_file("graphDB.postgres.port"),
              defaults.graph_db.postgres.port,
            ),
          ssl:
            boolean_p(
              from_env("GRAPH_DB_POSTGRES_SSL"),
              from_file("graphDB.postgres.ssl"),
              defaults.graph_db.postgres.ssl,
            ),
          username:
            non_empty_string_p(
              from_env("GRAPH_DB_POSTGRES_USERNAME"),
              from_file("graphDB.postgres.username"),
              defaults.graph_db.postgres.username,
            ),
          password:
            non_empty_string_p(
              from_env("GRAPH_DB_POSTGRES_PASSWORD"),
              from_file("graphDB.postgres.password"),
              defaults.graph_db.postgres.password,
            ),
          dbname:
            non_empty_string_p(
              from_env("GRAPH_DB_POSTGRES_DBNAME"),
              from_file("graphDB.postgres.dbname"),
              defaults.graph_db.postgres.dbname,
            ),
        },
      },
      timeseries_db: {
        connector:
          enum_p(
            [
              ("sqlite", SQLite),
              ("postgres", Postgres),
              ("elasticsearch", ElasticSearch),
            ],
            from_env("TIMESERIES_DB_CONNECTOR"),
            from_file("timeseriesDB.connector"),
            defaults.timeseries_db.connector,
          ),
        sqlite: {
          path:
            non_empty_string_p(
              from_env("TIMESERIES_DB_SQLITE_PATH"),
              from_file("timeseriesDB.sqlite.path"),
              defaults.timeseries_db.sqlite.path,
            ),
        },
        postgres: {
          host:
            non_empty_string_p(
              from_env("TIMESERIES_DB_POSTGRES_HOST"),
              from_file("timeseriesDB.postgres.host"),
              defaults.timeseries_db.postgres.host,
            ),
          port:
            unix_port_p(
              from_env("TIMESERIES_DB_POSTGRES_PORT"),
              from_file("timeseriesDB.postgres.port"),
              defaults.timeseries_db.postgres.port,
            ),
          ssl:
            boolean_p(
              from_env("TIMESERIES_DB_POSTGRES_SSL"),
              from_file("timeseriesDB.postgres.ssl"),
              defaults.timeseries_db.postgres.ssl,
            ),
          username:
            non_empty_string_p(
              from_env("TIMESERIES_DB_POSTGRES_USERNAME"),
              from_file("timeseriesDB.postgres.username"),
              defaults.timeseries_db.postgres.username,
            ),
          password:
            non_empty_string_p(
              from_env("TIMESERIES_DB_POSTGRES_PASSWORD"),
              from_file("timeseriesDB.postgres.password"),
              defaults.timeseries_db.postgres.password,
            ),
          dbname:
            non_empty_string_p(
              from_env("TIMESERIES_DB_POSTGRES_DBNAME"),
              from_file("timeseriesDB.postgres.dbname"),
              defaults.timeseries_db.postgres.dbname,
            ),
        },
        elasticsearch: {
          host:
            non_empty_string_p(
              from_env("TIMESERIES_DB_ELASTICSEARCH_HOST"),
              from_file("timeseriesDB.elasticsearch.host"),
              defaults.timeseries_db.elasticsearch.host,
            ),
          port:
            unix_port_p(
              from_env("TIMESERIES_DB_ELASTICSEARCH_PORT"),
              from_file("timeseriesDB.elasticsearch.port"),
              defaults.timeseries_db.elasticsearch.port,
            ),
          ssl:
            boolean_p(
              from_env("TIMESERIES_DB_ELASTICSEARCH_SSL"),
              from_file("timeseriesDB.elasticsearch.ssl"),
              defaults.timeseries_db.elasticsearch.ssl,
            ),
          username:
            optional_string_p(
              from_env("TIMESERIES_DB_ELASTICSEARCH_USERNAME"),
              from_file("timeseriesDB.elasticsearch.username"),
              defaults.timeseries_db.elasticsearch.username,
            ),
          password:
            optional_string_p(
              from_env("TIMESERIES_DB_ELASTICSEARCH_PASSWORD"),
              from_file("timeseriesDB.elasticsearch.password"),
              defaults.timeseries_db.elasticsearch.password,
            ),
          template_prefix:
            non_empty_string_p(
              from_env("TIMESERIES_DB_ELASTICSEARCH_TEMPLATE_PREFIX"),
              from_file("timeseriesDB.elasticsearch.templatePrefix"),
              defaults.timeseries_db.elasticsearch.template_prefix,
            ),
        },
      },
    };
  };
