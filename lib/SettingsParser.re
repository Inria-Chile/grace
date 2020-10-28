open Utils;

/** Locate an environment variable, either by it pointing to a file or
    if given the value directly. */
let env_find = (env, key) =>
  Option.map(read_file, env(Printf.sprintf("%s_FILE", key))) |? env(key);

/* Locate a nested key within a yaml value */
let rec yaml_find = (value, path) => {
  let entries =
    switch (value) {
    | `O(pairs) => pairs
    | _ => []
    };
  let path_parts = String.split_on_char('.', path);
  let entry = List.assoc_opt(List.hd(path_parts), entries);
  if (List.length(path_parts) === 1 || Option.is_none(entry)) {
    entry;
  } else {
    yaml_find(
      Option.get(entry),
      String.concat(String.make(1, '.'), List.tl(path_parts)),
    );
  };
};

/* type-of-other conversions */
let int_of_string: string => int = s => Int32.to_int(Int32.of_string(s));

let boolean_of_string: string => bool =
  s =>
    switch (String.lowercase_ascii(s)) {
    | "1"
    | "yes"
    | "y"
    | "true" => true
    | "0"
    | "no"
    | "n"
    | "false" => false
    | _ => failwith("boolean value improperly encoded")
    };

let boolean_of_float: float => bool =
  f =>
    if (f == 0.0) {
      false;
    } else if (f == 1.0) {
      true;
    } else {
      failwith("boolean value improperly encoded");
    };

/* Yaml value extractor functions */
let string_of_value: Yaml.value => string =
  value =>
    switch (value) {
    | `String(s) => s
    | _ => failwith("value is not a string")
    };

let int_of_value: Yaml.value => int =
  value =>
    switch (value) {
    | `Float(f) when Float.is_integer(f) => Float.to_int(f)
    | `String(s) => int_of_string(s)
    | _ => failwith("value is not an integer")
    };

let boolean_of_value: Yaml.value => bool =
  value =>
    switch (value) {
    | `Bool(b) => b
    | `Float(f) => boolean_of_float(f)
    | `String(s) => boolean_of_string(s)
    | _ => failwith("value is not a boolean")
    };

/* Parser for log level */
let log_level_p = (from_env, from_file, default) =>
  Option.map(Dolog.Log.level_of_string, from_env)
  |? Option.map(Dolog.Log.level_of_string % string_of_value, from_file)
  |? Some(default)
  |> Option.get;

/* Parser for regular string */
let string_p = (from_env, from_file, default) =>
  from_env |? Option.map(string_of_value, from_file) |? Some(default) |> Option.get;

/* Parser for enumerated string */
let enum_p = (options, from_env, from_file, default) => {
  let find_key = key =>
    try (List.assoc(key, options)) {
    | Not_found =>
      failwith(
        Printf.sprintf(
          "value %s is not a valid option; choices are {%s}",
          key,
          String.concat(", ", List.map(fst, options)),
        ),
      )
    };
  Option.map(find_key, from_env)
  |? Option.map(find_key % string_of_value, from_file)
  |? Some(default)
  |> Option.get;
};

/* Parser for non-empty string */
let non_empty_string_p = (from_env, from_file, default) => {
  let extract = s =>
    if (s == "") {
      failwith("value is an empty string");
    } else {
      s;
    };
  Option.map(extract, from_env)
  |? Option.map(extract % string_of_value, from_file)
  |? Some(default)
  |> Option.get;
};

/* Parser for optional string */
let optional_string_p = (from_env, from_file, default) =>
  switch (from_env, from_file) {
  | (Some(""), _) => None
  | (Some(s), _) => Some(s)
  | (None, Some(`String(""))) => None
  | (None, Some(`Null)) => None
  | (None, Some(`String(s))) => Some(s)
  | (None, None) => default
  | (None, _) => failwith("value is not a string")
  };

/* Parser for unix ports */
let unix_port_p = (from_env, from_file, default) => {
  let extract = n =>
    if (n > 0 && n < 65536) {
      n;
    } else {
      failwith("value is not a valid unix port (out of range)");
    };
  Option.map(extract % int_of_string, from_env)
  |? Option.map(extract % int_of_value, from_file)
  |? Some(default)
  |> Option.get;
};

/* Parser for boolean values */
let boolean_p = (from_env, from_file, default) =>
  Option.map(boolean_of_string, from_env)
  |? Option.map(boolean_of_value, from_file)
  |? Some(default)
  |> Option.get;

/* Parser for unix address values */
let unix_address_p = (from_env, from_file, default) =>
  Option.map(Unix.inet_addr_of_string, from_env)
  |? Option.map(Unix.inet_addr_of_string % string_of_value, from_file)
  |? Some(default)
  |> Option.get;
