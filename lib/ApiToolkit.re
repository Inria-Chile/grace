open Httpaf;
open Lwt.Infix;
module Log = Dolog.Log;

/* Simple wrapper for responses */
module Response = {
  /* A response is (for now), a triad of a fully-realized response, a
     set of headers, and a status code. */
  type t = (string, Headers.t, Status.standard);

  let replace_header: (Headers.name, Headers.value, Headers.t) => Headers.t =
    (name, value, headers) => Headers.add(Headers.remove(headers, name), name, value);

  /* Shortcuts for creating responses */
  let of_yojson: (~headers: Headers.t=?, ~status: Status.standard=?, Yojson.Safe.t) => t =
    (~headers=Headers.empty, ~status=`OK, body) => {
      let serialized = Yojson.Safe.to_string(body);
      let normalized_headers =
        headers
        |> replace_header("Content-Length", string_of_int(String.length(serialized)))
        |> replace_header("Content-Type", "application/json");
      (serialized, normalized_headers, status);
    };

  let ok: (~headers: Headers.t=?, Yojson.Safe.t) => t =
    (~headers=Headers.empty, body) => of_yojson(~headers, body);

  let created: (~headers: Headers.t=?, Yojson.Safe.t) => t =
    (~headers=Headers.empty, body) => of_yojson(~headers, ~status=`Created, body);

  let of_svg: (~headers: Headers.t=?, ~status: Status.standard=?, string) => t =
    (~headers=Headers.empty, ~status=`OK, body) => (
      body,
      headers
      |> replace_header("Content-Length", string_of_int(String.length(body)))
      |> replace_header("Content-Type", "image/svg+xml"),
      status,
    );

  module Error = {
    let make_error = status => {
      let error = `String(Status.default_reason_phrase(status));
      let code = `Int(Status.to_code((status :> Status.t)));
      (~details=`Null, ()) =>
        of_yojson(
          ~status,
          `Assoc([("error", error), ("code", code), ("details", details)]),
        );
    };

    let not_found = make_error(`Not_found);
    let bad_request = make_error(`Bad_request);
    let unauthorized = make_error(`Unauthorized);
    let forbidden = make_error(`Forbidden);
    let method_not_allowed = make_error(`Method_not_allowed);
    let internal_server_error = make_error(`Internal_server_error);
    let bad_gateway = make_error(`Bad_gateway);
  };
};

let path_syntax =
  Hashtbl.find(Neturl.common_url_syntax, "http") |> Neturl.partial_url_syntax;

/* Make a request handler for the httpaf server, given a function
   which performs the dispatching */
let make_request_handler = (handler, _, reqd) => {
  let url = Neturl.url_of_string(path_syntax, Reqd.request(reqd).target);
  Log.debug(
    "%s %s",
    Method.to_string(Reqd.request(reqd).meth),
    Neturl.string_of_url(url),
  );
  /* Normalize the path to avoid mismatching */
  let path = Neturl.url_path(url) |> Neturl.norm_path;
  /* Invoke the handler, and deal with the response or errors
     produced. */
  Lwt.async(() =>
    Lwt.catch(
      () => handler(Reqd.request(reqd).meth, path),
      ex =>
        Lwt.return(
          Response.Error.internal_server_error(
            ~details=`String(Printexc.to_string(ex)),
            (),
          ),
        ),
    )
    >|= (
      ((body, headers, status)) =>
        Reqd.respond_with_string(
          reqd,
          Httpaf.Response.create(~headers, (status :> Httpaf.Status.t)),
          body,
        )
    )
  );
};

/* The default error handler for the httpaf server */
let error_handler = (_, ~request as _=?, error, start_response) => {
  let error_response =
    switch (error) {
    | `Exn(ex) =>
      Response.Error.internal_server_error(~details=`String(Printexc.to_string(ex)), ())
    | `Bad_request => Response.Error.bad_request()
    | `Bad_gateway => Response.Error.bad_gateway()
    | `Internal_server_error => Response.Error.internal_server_error()
    };
  let (body, headers, _) = error_response;
  let response_body = start_response(headers);
  Body.write_string(response_body, body);
  Body.close_writer(response_body);
};

/* The httpaf server */
let server = (handler, addr, port) =>
  Httpaf_lwt_unix.(
    Lwt_io.establish_server_with_client_socket(
      Unix.(ADDR_INET(addr, port)),
      Server.create_connection_handler(
        ~request_handler=make_request_handler(handler),
        ~error_handler,
      ),
    )
  );
