open Grace;

let settings = Alcotest.testable(Settings.pp, Settings.equal);

let test_barebones_initialize = () =>
  Alcotest.(
    check(
      settings,
      "same settings",
      Settings.defaults,
      Settings.initialize(_ => None, None),
    )
  );

let test_settings_from_file = () =>
  Alcotest.(
    check(
      settings,
      "same settings",
      {
        log_level: Dolog.Log.WARN,
        execution_mode: Settings.Coordinator,
        consumer: {
          ...Settings.defaults.consumer,
          connector: Settings.Redis,
          redis: {
            ...Settings.defaults.consumer.redis,
            host: "redis",
          },
        },
        forwarder: {
          ...Settings.defaults.forwarder,
          connector: Settings.Redis,
          redis: {
            ...Settings.defaults.forwarder.redis,
            host: "redis",
          },
        },
        graph_api: {
          ...Settings.defaults.graph_api,
          listen_address: Unix.inet_addr_of_string("0.0.0.0"),
        },
        work_api: {
          ...Settings.defaults.work_api,
          listen_address: Unix.inet_addr_of_string("0.0.0.0"),
        },
        graph_db: {
          ...Settings.defaults.graph_db,
          connector: Settings.Postgres,
          postgres: {
            ...Settings.defaults.graph_db.postgres,
            host: "postgres",
          },
        },
        timeseries_db: {
          ...Settings.defaults.timeseries_db,
          connector: Settings.ElasticSearch,
          elasticsearch: {
            ...Settings.defaults.timeseries_db.elasticsearch,
            host: "elasticsearch",
            username: Some("grace"),
            password: Some("grace"),
          },
        },
      },
      Settings.initialize(_ => None, Some("data/test-settings.yaml")),
    )
  );

let tests = [
  Alcotest.test_case("barebones_initialize", `Quick, test_barebones_initialize),
  Alcotest.test_case("settings_from_file", `Quick, test_settings_from_file),
];
