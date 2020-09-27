open Lwt.Infix;
module Log = Dolog.Log;
module Settings = Settings;
module TemplateGraph = TemplateGraph;
module Utils = Utils;
module Version = Version;

type application = {
  .
  settings: Settings.t,
  run: unit => Lwt.t(unit),
  stop: unit => unit,
};

let current_application = ref(None);

let react_to_signals = () => {
  let _ =
    Lwt_unix.on_signal(Sys.sigint, _ =>
      switch (current_application^) {
      | None => ()
      | Some(a) =>
        Log.info("%s", "Stopping grace server");
        a#stop();
      }
    );
  let _ =
    Lwt_unix.on_signal(Sys.sigterm, _ =>
      switch (current_application^) {
      | None => ()
      | Some(a) =>
        Log.info("%s", "Stopping grace server");
        a#stop();
      }
    );
  ();
};

let app: Settings.t => application =
  s => {
    val running = ref(false);
    val finish = ref(None);
    pub settings = s;
    pub run = () =>
      if (running^) {
        Log.warn("Application is already running");
        Lwt.return();
      } else if (Option.is_some(current_application^)) {
        Log.error("Another application is already running");
        Lwt.return();
      } else {
        /* Register application */
        current_application := Some(this);
        running := true;
        /* Set up the logger */
        Log.set_output(stdout);
        Log.set_log_level(this#settings.log_level);
        /* Start services */
        Log.info("Starting services");
        ApiToolkit.server(
          (_, _) => Lwt.return(ApiToolkit.Response.ok(`String("Hello World!"))),
          this#settings.api.listen_address,
          this#settings.api.listen_port,
        )
        >>= (
          srv => {
            Log.info(
              "Started grace server listening to %s:%d",
              Unix.string_of_inet_addr(this#settings.api.listen_address),
              this#settings.api.listen_port,
            );
            let (cycle, resolver) = Lwt.task();
            finish := Some(resolver);
            cycle >>= (() => Lwt_io.shutdown_server(srv));
          }
        );
      };
    pub stop = () =>
      if (! running^) {
        Log.warn("Application isn't running; nothing to do");
      } else {
        /* Stop services */
        Log.info("Stopping services");
        Lwt.wakeup_later(Option.get(finish^), ());
        /* Unregister application */
        running := false;
        current_application := None;
        Log.info("Application has stopped");
      }
  };
