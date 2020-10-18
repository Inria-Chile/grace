open ApiToolkit;

let liveness = () =>
  Lwt.return(
    Response.ok(
      `Assoc([
        ("status", `String("pass")),
        ("version", `String(Version.version)),
        ("releaseId", `String(Version.sha1)),
        ("description", `String("GRAph-based Computation Engine")),
        ("checks", `Assoc([])),
      ]),
    ),
  );

let readyness = () =>
  Lwt.return(Response.Error.internal_server_error(~kind="NotImplemented", ()));
