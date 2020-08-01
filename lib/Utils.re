/* Function composition */
let (%): ('b => 'c, 'a => 'b, 'a) => 'c = (f, g, x) => f(g(x));

/* OR for option values */
let (|?): (option('a), option('a)) => option('a) =
  (oa, ob) =>
    switch (oa, ob) {
    | (None, b) => b
    | (a, _) => a
    };

/* AND for option values */
let (|&): (option('a), option('a)) => option('a) =
  (oa, ob) =>
    switch (oa, ob) {
    | (None, _) => None
    | (_, None) => None
    | (Some(_), Some(b)) => Some(b)
    };

/* OR for result values */
let ($?): (result('a, string), result('a, string)) => result('a, string) =
  (ra, rb) =>
    switch (ra, rb) {
    | (Error(ea), Error(eb)) => Error(ea ++ "; " ++ eb)
    | (Error(_), Ok(b)) => Ok(b)
    | (Ok(a), _) => Ok(a)
    };

/* AND for result values */
let ($&): (result('a, string), result('a, string)) => result('a, string) =
  (ra, rb) =>
    switch (ra, rb) {
    | (Ok(_), Ok(b)) => Ok(b)
    | (Error(e), Ok(_)) => Error(e)
    | (Ok(_), Error(e)) => Error(e)
    | (Error(ea), Error(eb)) => Error(ea ++ "; " ++ eb)
    };

/** Directed Acyclic Graph utilities */
module DAG = {
  /** Alias for common topological graph structure */
  type t = list((string, list(string)));

  module NodeSet = Set.Make(String);

  /** Generate a list of independent layers of the graph */
  let slices: t => list(list(string)) =
    edges => {
      let pool = NodeSet.of_seq(List.to_seq(edges) |> Seq.map(fst));
      let consumed = ref(NodeSet.empty);
      let changed = ref(true);
      let slice_collection = ref([]);
      while (!NodeSet.equal(pool, consumed^) && changed^) {
        let slice =
          List.to_seq(edges)
          |> Seq.filter(((_, linked)) =>
               List.for_all(n => NodeSet.mem(n, consumed^), linked)
             )
          |> Seq.map(fst)
          |> NodeSet.of_seq;
        changed := !NodeSet.is_empty(slice);
        consumed := NodeSet.union(consumed^, slice);
        slice_collection := [List.of_seq(NodeSet.to_seq(slice)), ...slice_collection^];
      };
      if (!NodeSet.is_empty(pool) && ! changed^) {
        failwith(
          Printf.sprintf(
            "The graph is cyclical (remaining nodes are {%s})",
            NodeSet.diff(pool, consumed^)
            |> NodeSet.to_seq
            |> List.of_seq
            |> String.concat(", "),
          ),
        );
      };
      List.rev(slice_collection^);
    };

  /** Compute graph loops and invalid references */
  let inconsistencies: t => option(string) =
    edges => {
      let pool = NodeSet.of_seq(List.to_seq(edges) |> Seq.map(fst));
      if (NodeSet.cardinal(pool) != List.length(edges)) {
        let duplicates = ref(NodeSet.empty);
        let sorted =
          List.map(fst, edges) |> List.fast_sort(String.compare) |> Array.of_list;
        let () =
          sorted
          |> Array.iteri((index, node) =>
               if (index > 0 && sorted[index - 1] == node) {
                 duplicates := NodeSet.add(node, duplicates^);
               }
             );
        Some(
          Printf.sprintf(
            "Some graph nodes are duplicated: {%s}",
            duplicates^ |> NodeSet.to_seq |> List.of_seq |> String.concat(", "),
          ),
        );
      } else {
        let broken_references =
          List.to_seq(edges)
          |> Seq.flat_map(((_, linked)) =>
               List.to_seq(linked) |> Seq.filter(n => !NodeSet.mem(n, pool))
             )
          |> NodeSet.of_seq;
        if (!NodeSet.is_empty(broken_references)) {
          Some(
            Printf.sprintf(
              "The graph contains broken references: {%s}",
              NodeSet.to_seq(broken_references) |> List.of_seq |> String.concat(", "),
            ),
          );
        } else {
          switch (slices(edges)) {
          | exception (Failure(msg)) => Some(msg)
          | _ => None
          };
        };
      };
    };
};
