let () =
  Alcotest.run(
    "GRACE unit tests",
    [("Settings", TestSettings.tests), ("TemplateGraph", TestTemplateGraph.tests)],
  );
