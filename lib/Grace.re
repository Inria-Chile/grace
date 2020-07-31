module Log = Dolog.Log;
module Settings = Settings;
module TemplateGraph = TemplateGraph;
module Version = Version;

type application = {
  .
  settings: Settings.t,
  run: unit => unit,
  stop: unit => unit,
};

let current_application = ref(None);

let app: Settings.t => application =
  s => {
    val running = ref(false);
    pub settings = s;
    pub run = () =>
      if (running^) {
        Log.warn("Application is already running");
      } else if (Option.is_some(current_application^)) {
        Log.error("Another application is already running");
      } else {
        /* Register application */
        current_application := Some(this);
        running := true;
        /* Set up the logger */
        Log.set_output(stdout);
        Log.set_output(stdout);
        Log.set_log_level(this#settings.log_level);
        /* Start services */
        Log.info("Starting services");
        /* TODO start services */
        Log.info("Application has started");
      };
    pub stop = () =>
      if (! running^) {
        Log.warn("Application isn't running; nothing to do");
      } else {
        /* Stop services */
        Log.info("Stopping services");
        /* TODO stop services */
        /* Unregister application */
        running := false;
        current_application := None;
        Log.info("Application has stopped");
      }
  };
