open Grace;

let serialize_slices =
  List.map(nodes => List.map((node: TemplateGraph.Node.t) => node.cname, nodes));

let base_node: TemplateGraph.Node.t = {
  cname: "ignored",
  name: "Ignored",
  description: None,
  domain: {
    entity: "ignored",
    which: TemplateGraph.DomainQuery.And([]),
  },
  inputs: [],
  invocation: TemplateGraph.Node.Command("dummy-command"),
  meta: `Null,
};

let base_input: TemplateGraph.Node.input = {
  cname: "ignored",
  relation: TemplateGraph.Node.Bijection,
};

let test_valid_regular_dag = () =>
  Alcotest.(
    check(
      list(list(string)),
      "same slices",
      [["bar", "foo"], ["baz"], ["ipsum", "lorem"]],
      TemplateGraph.dag_slices({
        cname: "regular",
        nodes: [
          {...base_node, cname: "lorem", inputs: [{...base_input, cname: "baz"}]},
          {
            ...base_node,
            cname: "ipsum",
            inputs: [{...base_input, cname: "baz"}, {...base_input, cname: "foo"}],
          },
          {...base_node, cname: "foo", inputs: []},
          {...base_node, cname: "bar", inputs: []},
          {...base_node, cname: "baz", inputs: [{...base_input, cname: "bar"}]},
        ],
      })
      |> serialize_slices,
    )
  );

let test_valid_empty_dag = () =>
  Alcotest.(
    check(
      list(list(string)),
      "same slices",
      [],
      TemplateGraph.dag_slices({cname: "empty", nodes: []}) |> serialize_slices,
    )
  );

let test_invalid_cyclical_graph = () =>
  Alcotest.(
    check_raises(
      "detects cycles in a graph",
      Failure("The graph is cyclical (remaining nodes are {bar, baz, foo})"),
      () => {
        let _ =
          TemplateGraph.dag_slices({
            cname: "empty",
            nodes: [
              {...base_node, cname: "lorem", inputs: [{...base_input, cname: "ipsum"}]},
              {...base_node, cname: "ipsum", inputs: []},
              {...base_node, cname: "foo", inputs: [{...base_input, cname: "baz"}]},
              {...base_node, cname: "bar", inputs: [{...base_input, cname: "foo"}]},
              {...base_node, cname: "baz", inputs: [{...base_input, cname: "bar"}]},
            ],
          });
        ();
      },
    )
  );

let json = Alcotest.testable(Yojson.Safe.pp, Yojson.Safe.equal);

let test_correctly_json_encoded = () => {
  let base = Yojson.Safe.from_string(Utils.read_file("data/test-template-graph.json"));
  Alcotest.(
    check(
      json,
      "correctly encoded",
      base,
      TemplateGraph.to_yojson(
        switch (TemplateGraph.of_yojson(base)) {
        | Ok(graph) => graph
        | Error(e) => failwith(e)
        },
      ),
    )
  );
};

let tests = [
  Alcotest.test_case("valid_regular_dag", `Quick, test_valid_regular_dag),
  Alcotest.test_case("valid_empty_dag", `Quick, test_valid_empty_dag),
  Alcotest.test_case("invalid_cyclical_graph", `Quick, test_invalid_cyclical_graph),
  Alcotest.test_case("correctly_json_encoded", `Quick, test_correctly_json_encoded),
];
