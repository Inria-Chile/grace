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
  } else if (CLIOptionMap.mem("--show-settings", opts)) {
    print_endline("TODO print settings");
  };
};

let () = main();
