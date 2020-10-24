open Utils;

/** A [DomainQuery.t] is an encoded representation of a subset of
    domain entities, queried by selecting their attributes,
    relationshipts between them, or both combined in boolean
    [DomainQuery.And] or [DomainQuery.Or] chains. */
module DomainQuery = {
  [@deriving yojson]
  type t = {
    entity: string,
    [@default And([])]
    which: query_expression,
  }
  and query_expression =
    | And(list(query_expression))
    | Or(list(query_expression))
    | Not(query_expression)
    | Filter(filter)
  and filter = {
    [@default None]
    forward: option(string),
    [@default None]
    backward: option(string),
    [@default None]
    query: option(t),
    [@key "attrEq"] [@default None]
    attr_eq: option((string, Yojson.Safe.t)),
  };

  /** Validate the consistency of a domain query (it makes sense, it
      can be used). */
  let rec validate_filter: filter => result(filter, string) =
    f =>
      switch (f.forward, f.backward, f.query, f.attr_eq) {
      /* Nothing is specified, the filter is useless */
      | (None, None, None, None) => Error("DomainQuery.filter is empty")
      /* It's a simple attribute filter */
      | (None, None, None, Some(_)) => Ok(f)
      /* It's a relation to other entities */
      | (Some(_), None, Some(q), None)
      | (None, Some(_), Some(q), None) =>
        switch (validate(q)) {
        | Error(e) => Error(e)
        | Ok(_) => Ok(f)
        }
      /* No relation was specified, only the target entity */
      | (None, None, Some(_), None) =>
        Error("DomainQuery.filter is missing a forward or backward relation")
      /* An ambiguous combination was specified */
      | (_, _, _, _) =>
        Error(
          "DomainQuery.filter is ambiguous (only one of {forward, backward, attrEq} may be used)",
        )
      }
  and validate_query_expression: query_expression => result(query_expression, string) =
    expr =>
      switch (expr) {
      /* An empty AND or OR chain is perfectly valid */
      | And([])
      | Or([]) => Ok(expr)
      /* A non empty AND or OR chain is checked item by item */
      | And([headq, ...tailq])
      | Or([headq, ...tailq]) =>
        switch (
          List.map(validate_query_expression, tailq)
          |> List.fold_left(($&), validate_query_expression(headq))
        ) {
        | Error(e) => Error(e)
        | Ok(_) => Ok(expr)
        }
      /* A NOT is valid if the expression being negated is valid */
      | Not(q) =>
        switch (validate_query_expression(q)) {
        | Error(e) => Error(e)
        | Ok(_) => Ok(expr)
        }
      /* A concrete filter expression is checked by itself */
      | Filter(f) =>
        switch (validate_filter(f)) {
        | Error(e) => Error(e)
        | Ok(_) => Ok(expr)
        }
      }
  and validate: t => result(t, string) =
    query =>
      switch (validate_query_expression(query.which)) {
      | Error(e) => Error(e)
      | Ok(_) => Ok(query)
      };
};

/** A [Node.t] is a single member of a template graph. It represents a
    variable with standalone computation rules and a specific set of
    domain entities to spread over. A node points to its inputs as
    references to other nodes and the relationship between the domain
    entities of itself and each input. */
module Node = {
  [@deriving yojson]
  type relation =
    | Bijection
    | Forward(string)
    | Backward(string);

  [@deriving yojson]
  type input = {
    cname: string,
    relation,
  };

  [@deriving yojson]
  type invocation =
    | Lua(string)
    | Command(string)
    | Remote(string);

  [@deriving yojson]
  type t = {
    cname: string,
    name: string,
    [@default None]
    description: option(string),
    domain: DomainQuery.t,
    inputs: list(input),
    invocation,
    [@default Yojson.Safe.(`Null)]
    meta: Yojson.Safe.t,
  };

  /** A node can only be invalid if it's domain query is invalid. */
  let validate: t => result(t, string) =
    node =>
      switch (DomainQuery.validate(node.domain)) {
      | Error(e) => Error(e)
      | Ok(_) => Ok(node)
      };
};

/** A [TemplateGraph.t] is a collection of nodes under a single
    canonical name, which serves as a namespace for them. The
    collection is consistent in that they form a directed acyclic
    graph, and each node is well defined. */
[@deriving yojson]
type t = {
  cname: string,
  nodes: list(Node.t),
};

let to_dag: t => DAG.t =
  graph =>
    List.map(
      (n: Node.t) => (n.cname, List.map((i: Node.input) => i.cname, n.inputs)),
      graph.nodes,
    );

let validate: t => result(t, string) =
  graph =>
    switch (graph.nodes) {
    | [] => Ok(graph)
    | [n, ...ns] =>
      switch (List.map(Node.validate, ns) |> List.fold_left(($&), Node.validate(n))) {
      | Error(e) => Error(e)
      | _ =>
        switch (DAG.inconsistencies(to_dag(graph))) {
        | Some(inconsistencies) => Error(inconsistencies)
        | None => Ok(graph)
        }
      }
    };

let dag_slices: t => list(list(Node.t)) =
  graph =>
    DAG.slices(to_dag(graph))
    |> List.map(names =>
         List.map(
           name => List.find((node: Node.t) => node.cname == name, graph.nodes),
           names,
         )
       );

module Service = {
  open ApiToolkit;

  let invalid_template_graph = (cname, error) =>
    raise(
      Request.InvalidParameter(
        cname,
        Printf.sprintf("improperly encoded template graph: %s", error),
      ),
    );

  let validate: (string, Yojson.Safe.t) => Yojson.Safe.t =
    (cname, body) =>
      switch (Result.bind(of_yojson(body), validate)) {
      | Ok(template_graph) => to_yojson(template_graph)
      | Error(e) => invalid_template_graph(cname, e)
      };

  let dag_slices: (string, Yojson.Safe.t) => Yojson.Safe.t =
    (cname, body) =>
      switch (of_yojson(body)) {
      | Ok(template_graph) =>
        dag_slices(template_graph)
        |> List.map(slice => `List(List.map(Node.to_yojson, slice)))
        |> (slices => `List(slices))
      | Error(e) => invalid_template_graph(cname, e)
      };
};

module Controller = {
  open ApiToolkit;
  open Lwt.Infix;

  /* Decode-encode a given template graph */
  let echo: Request.t => Lwt.t(Response.t) =
    req => Request.body(req) >|= Service.validate("_echo") >|= Response.ok;

  /* Extract DAG slices given a template graph */
  let echo_dag_slices: Request.t => Lwt.t(Response.t) =
    req => Request.body(req) >|= Service.dag_slices("_echo") >|= Response.ok;
};
