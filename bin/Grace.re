module CLIOptionMap = Map.Make(String);

let collect_options: unit => CLIOptionMap.t(string) =
  () => {
    let opts = ref(CLIOptionMap.empty);
    Arg.parse(
      [
        (
          "--version",
          Arg.Unit(() => opts := CLIOptionMap.add("--version", "requested", opts^)),
          "Show version and exit",
        ),
        (
          "--show-settings",
          Arg.Unit(
            () => opts := CLIOptionMap.add("--show-settings", "requested", opts^),
          ),
          "Show settings and exit",
        ),
        (
          "--settings",
          Arg.String(path => opts := CLIOptionMap.add("--settings", path, opts^)),
          "Use the specified settings file",
        ),
      ],
      _ => (),
      "Usage: grace [OPTION]...",
    );
    opts^;
  };

let main = () => {
  let opts = collect_options();
  if (CLIOptionMap.mem("--version", opts)) {
    print_endline(
      Printf.sprintf(
        "GRACE version %s build %s sha1 %s",
        Grace.Version.version,
        Grace.Version.build,
        Grace.Version.sha1,
      ),
    );
  } else {
    let settings =
      Grace.Settings.initialize(CLIOptionMap.find_opt("--settings", opts));
    if (CLIOptionMap.mem("--show-settings", opts)) {
      print_endline(Grace.Settings.show(settings));
    } else {
      Grace.app(settings)#run();
    };
  };
};

let () = main();
