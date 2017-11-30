let _toArray = Array.of_list;
open Rebase;

module Message = {
  type send =
    | Run(list((Test.id, string)));

  type receive =
    | CaseCycle(Test.id, Test.result)
    | SuiteCycle(Test.id, Test.result)
    | SuiteComplete;

  let _decodeReceived = message => {
    let data = message##data##contents;

    let makeResult = () => Test.{
      hz: data##hz,
      sampleCount: data##sampleCount,
      rme: data##rme,
    };

    switch message##data##_type {
    
    | "caseCycle" =>
      CaseCycle(data##id, makeResult())

    | "suiteCycle" =>
      SuiteCycle(data##id, makeResult())

    | "complete" =>
      SuiteComplete

    | _ =>
      failwith("unknown worker message")
    }
  };
  
  let _encodeToSend : send => array({. "name": Test.id, "code": string }) =
    fun | Run(tests) =>
            tests |> _toArray
                  |> Js.Array.reverseInPlace
                  |> Array.map(((id, code)) => {
                        "name": id,
                        "code"
                      })
};

type _worker;

[@bs.new] external _makeWorker : string => _worker = "Worker";
[@bs.set] external _onmessage : _worker => (Js.t({..}) => unit) => unit = "onmessage";
[@bs.set] external _onerror : _worker => (string => unit) => unit = "onerror";
[@bs.send] external _postMessage : _worker => 'a => unit = "postMessage";

type t = {
  postMessage: Message.send => unit
};

let make = (~onMessage, ~onError) => {
  let timeoutId = ref(None);
  let worker = _makeWorker("../build/worker.js");

  _onmessage(worker, message => {
    if (message##_type == "end") {
      timeoutId.contents |> Option.forEach(Js.Global.clearTimeout);
    } else {
      message |> Message._decodeReceived
              |> onMessage;
    }
  });
  _onerror(worker, onError);

  {
    postMessage: message =>
      message |> Message._encodeToSend
              |> _postMessage(worker)
  }
};