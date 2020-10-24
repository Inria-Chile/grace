open ApiToolkit;

let route_not_found = (root, method, path) =>
  Lwt.return(
    Response.Error.not_found(
      ~kind="RouteNotFound",
      ~details=
        Some(
          Printf.sprintf(
            "The route %s %s doesn't match any server resource",
            Httpaf.Method.to_string(method),
            Neturl.join_path([""] @ root @ path),
          ),
        ),
      (),
    ),
  );

/* The handler for /api/v1 endpoints */
let api_v1_handler = (root, settings, method, path, req) =>
  switch (method, path) {
  /* API's health */
  | (`GET, ["healthz"])
  | (`GET, ["health"])
  | (`GET, ["healthz", "live"])
  | (`GET, ["health", "live"]) => Healthchecks.Controller.liveness()
  | (`GET, ["healthz", "ready"])
  | (`GET, ["health", "ready"]) => Healthchecks.Controller.readyness(settings)
  /* Graph templates */
  /* Reject GETs to the _echo special template */
  | (`GET, ["templates", "_echo", ..._]) =>
    Lwt.return(
      Response.Error.method_not_allowed(
        ~kind="EchoTemplate",
        ~details=Some("the _echo template must be given as the request body via POST"),
        (),
      ),
    )
  | (`POST, ["templates", "_echo"]) => TemplateGraph.Controller.echo(req)
  | (`POST, ["templates", "_echo", "dag-slices"]) =>
    TemplateGraph.Controller.echo_dag_slices(req)
  | _ => route_not_found(root, method, path)
  };

/* The root handler */
let handler: (Settings.t, Httpaf.Method.t, list(string), Request.t) => Lwt.t(Response.t) =
  (settings, method, path, req) =>
    switch (path) {
    | ["api", "v1", ...rest] => api_v1_handler(["api", "v1"], settings, method, rest, req)
    | _ => route_not_found([], method, path)
    };
