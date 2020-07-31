/* Type scaffolding */

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

  let validate: t => result(t, string) = _ => Error("not implemented");
};

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
  type t = {
    cname: string,
    name: string,
    description: option(string),
    domain: DomainQuery.t,
    inputs: list(input),
    meta: Yojson.Safe.t,
  };

  let validate: t => result(t, string) = _ => Error("not implemented");
};

module Graph = {
  [@deriving yojson]
  type t = list(Node.t);

  let validate: t => result(t, string) = _ => Error("not implemented");
};
