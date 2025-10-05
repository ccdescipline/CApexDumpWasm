import createModule from "./DumpWasm.js"; // 或相对路径

let Module = null;

(async () => {
  Module = await createModule();
  self.postMessage({ type: "ready" });
})();

self.onmessage = async (e) => {
  const { type, fileArray } = e.data;
  if (type === "runHeavy") {

    // 把file转成地址
    const fileptr = Module._malloc(fileArray.length);
    Module.HEAPU8.set(fileArray, fileptr);

    const outputPtr = Module._malloc(8); // 分配指针的空间
    console.log(outputPtr);

    const outputErrorPtr = Module._malloc(8); // 分配指针的空间
    console.log(outputErrorPtr);

    let status;

    status = Module._dumpAll(fileptr, fileArray.length, outputPtr, outputErrorPtr);

    const resultPtr = Module.HEAP32[outputPtr / 4];
    const errorPtr = Module.HEAP32[outputErrorPtr / 4];

    Module._free(fileptr);
    Module._free(outputPtr);
    Module._free(outputErrorPtr);

    let parsetJson = JSON.parse(Module.UTF8ToString(resultPtr));
    let errorJson = JSON.parse(Module.UTF8ToString(errorPtr));
    
    
    postMessage({ type, parsetJson, errorJson});
  }
};
