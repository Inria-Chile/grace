open Lwt.Infix;
open ApiToolkit;

module Service = {
  let perform_checks:
    (
      ~consumer: option(Settings.consumer)=?,
      ~forwarder: option(Settings.forwarder)=?,
      ~graph_db: option(Settings.graph_db)=?,
      ~timeseries_db: option(Settings.timeseries_db)=?,
      unit
    ) =>
    Lwt.t((string, Yojson.Safe.t)) =
    (~consumer=None, ~forwarder=None, ~graph_db=None, ~timeseries_db=None, ()) =>
      switch (consumer, forwarder, graph_db, timeseries_db) {
      | (None, None, None, None) =>
        Lwt.return((
          "pass",
          `Assoc([
            ("status", `String("pass")),
            ("version", `String(Version.version)),
            ("releaseId", `String(Version.sha1)),
            ("description", `String("GRAph-based Computation Engine")),
            ("checks", `Assoc([])),
          ]),
        ))
      | _ => Lwt.fail(Request.NotImplemented)
      };
};

module Controller = {
  let health_to_response = ((status, health)) =>
    switch (status) {
    | "pass"
    | "warn" => Response.ok(health)
    | _ => Response.of_yojson(~status=`Internal_server_error, health)
    };

  /* Liveness check means the application is running */
  let liveness: unit => Lwt.t(Response.t) =
    () => Service.perform_checks() >|= health_to_response;

  /* Readyness check means the application is ready to handle requests */
  let readyness: Settings.t => Lwt.t(Response.t) =
    settings =>
      Service.perform_checks(
        ~consumer=Some(settings.consumer),
        ~forwarder=Some(settings.forwarder),
        ~graph_db=Some(settings.graph_db),
        ~timeseries_db=Some(settings.timeseries_db),
        (),
      )
      >|= health_to_response;
};
