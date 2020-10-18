open ApiToolkit;
module Log = Dolog.Log;

let handler: (Httpaf.Method.t, list(string), Request.t) => Lwt.t(Response.t) =
  (method, path, _) => {
    Log.debug("%s %s", Httpaf.Method.to_string(method), Neturl.join_path(path));
    switch (method, path) {
    | (`GET, ["api", "v1", "healthz"])
    | (`GET, ["api", "v1", "health"])
    | (`GET, ["api", "v1", "healthz", "live"])
    | (`GET, ["api", "v1", "health", "live"]) => Healthchecks.liveness()
    | (`GET, ["api", "v1", "healthz", "ready"])
    | (`GET, ["api", "v1", "health", "ready"]) => Healthchecks.readyness()
    | _ => Lwt.return(Response.Error.not_found(~kind="RouteNotFound", ()))
    };
  };
