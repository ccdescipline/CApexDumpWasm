<script setup>
import { onMounted, ref } from "vue";
import DumpWasmModule from "../wasm/DumpWasm.js";
import { ElMessage } from 'element-plus'

let CmakeModuleInstanse;

const dumpMsg = ref("");
const dumpObj = ref({});
const searchText = ref("");
const searchList = ref([]);
const textarea = ref(null); // 定义 ref
const isDump = ref(false);
let worker = null;
const loading = ref(false)


onMounted(async () => {
  console.log("hello");

  // const moduleInstance = await MainModule({ locateFile: (file) => myWasmUrl });
  // console.log(moduleInstance); // 输出 5



  CmakeModuleInstanse = await DumpWasmModule();
  let ressult = CmakeModuleInstanse._add(2, 3);
  console.log(ressult); // 输出 5
  console.log(CmakeModuleInstanse);


  worker = new Worker(new URL('../wasm/wasmWorker.js', import.meta.url), { type: 'module' });

   worker.onmessage = (e) => {
    const { type, parsetJson, errorJson } = e.data;
    if (type === 'runHeavy') {
      dumpObj.value = parsetJson;
      console.log("worker onmessage")

      console.log(parsetJson);
      console.log("errorJson", errorJson);
      formatDump();

      isDump.value = true; // 设置为 true，表示已经 dump 过了
      loading.value = false;
    }else if(type ==='error'){
      ElMessage.error('An error has occurred. Please check the console for details');
      loading.value = false;
    }
    

  };

  console.log(CmakeModuleInstanse.HEAPU8.buffer);
  
});

const formatDump = function (dumpField) {
  dumpMsg.value = `${formatMics(dumpObj.value["Mics"])}

${formatButtons(dumpObj.value["Buttons"])}

${formatRecvTable(dumpObj.value["RecvTable"])}

${formatConvar(dumpObj.value["Convars"])}

${formatdataMap(dumpObj.value["dataMap"])}

${formatWeaponSettings(dumpObj.value["weaponSettings"])}
`;
};

const formatMics = function (ConVarsObj) {
  const ConvarformattedData = Object.entries(ConVarsObj).map(
    ([key, value], index) => {
      return `${key == "" ? "{empty}" : key} = 0x${value.toString(16)}`;
    }
  );

  return `[Mics] \n${ConvarformattedData.join("\n")}`;
};

const formatConvar = function (ConVarsObj) {
  const ConvarformattedData = Object.entries(ConVarsObj).map(
    ([key, value], index) => {
      return `${key == "" ? "{empty}" : key} = 0x${value.toString(16)}`;
    }
  );

  return `[Convars] \n${ConvarformattedData.join("\n")}`;
};

const formatWeaponSettings = function (ConVarsObj) {
  const ConvarformattedData = Object.entries(ConVarsObj).map(
    ([key, value], index) => {
      return `${key == "" ? "{empty}" : key} = 0x${value.toString(16)}`;
    }
  );

  return `[weaponSettings] \n${ConvarformattedData.join("\n")}`;
};

const formatButtons = function (ConVarsObj) {
  const ConvarformattedData = Object.entries(ConVarsObj).map(
    ([key, value], index) => {
      return `${key == "" ? "{empty}" : key} = 0x${value.toString(16)}`;
    }
  );

  return `[Buttons] \n${ConvarformattedData.join("\n")}`;
};

const formatRecvTable = function (RecvTableObj) {
  const recvTableformattedData = Object.entries(RecvTableObj).map(
    ([key, value]) => {
      const tableinfo = Object.entries(value).map(([key, value]) => {
        return `${key} = 0x${value.toString(16)}`;
      });

      return `[${key}]\n${tableinfo.join("\n")}\n`;
    }
  );

  return `[RecvTable] \n${recvTableformattedData.join("\n")}`;
};

const formatdataMap = function (RecvTableObj) {
  const recvTableformattedData = Object.entries(RecvTableObj).map(
    ([key, value]) => {
      const tableinfo = Object.entries(value).map(([key, value]) => {
        return `${key} = 0x${value.toString(16)}`;
      });

      return `[${key}]\n${tableinfo.join("\n")}\n`;
    }
  );

  return `[DataMap] \n${recvTableformattedData.join("\n")}`;
};

const handleFileChange = async (file, filelist) => {
  if (file) {
    const arrayBuffer = await file.raw.arrayBuffer(); // 注意：要用 file.raw
    const fileArray = new Uint8Array(arrayBuffer);

    loading.value = true;
    worker.postMessage({
      type: 'runHeavy',
      fileArray
    });

    // isDump.value = true; // 设置为 true，表示已经 dump 过了
    //console.log();
  }
};

function handleInputChange(value) {
  if (value == "") {
    searchList.value = [];
    return;
  }

  //console.log('输入框内容变化了：', value)
  searchList.value = dumpMsg.value
    .split("\n")
    .map((item, index) => ({ index, text: item })) // 先把每一行变成 {index, text}
    .filter((item) => item.text.toLowerCase().includes(value.toLowerCase())); // 然后过滤
}

function highlight(text) {
  const escapeRegExp = function (string) {
    return string.replace(/[.*+?^${}()|[\]\\]/g, "\\$&");
  };

  if (!searchText.value) return text; // 如果没搜索，不处理

  // 先转义用户输入，避免正则报错
  const escaped = escapeRegExp(searchText.value);

  const regex = new RegExp(`(${escaped})`, "gi");
  return text.text.replace(regex, `<span style="color: red;">$1</span>`);
}

function jumpToKeyword(line) {
  // Scroll the textarea to the target position
  //textarea.scrollTop = scrollPosition;
  console.log(textarea.value);
  textarea.value.scrollTop = line * 18 + 10;
}

const handleExport = function () {
  // 将 JSON 对象转换为字符串
  const jsonString = JSON.stringify(dumpObj.value, null, 2); // 第二个参数是 replacer，第三个参数是缩进空格数

  // 创建一个 Blob 对象
  const blob = new Blob([jsonString], { type: "application/json" });

  // 创建一个临时的 <a> 标签
  const link = document.createElement("a");

  // 使用 URL.createObjectURL 生成下载链接
  link.href = URL.createObjectURL(blob);

  // 指定下载的文件名
  link.download = "offset.json";

  // 触发下载
  link.click();

  // 清理 URL 对象
  URL.revokeObjectURL(link.href);
};
</script>

<template>
  <div class="content" v-loading="loading">
    <div class="content-top">
      <el-upload
        action=""
        :auto-upload="false"
        :on-change="handleFileChange"
        :show-file-list="false"
      >
        <el-button type="primary">Select File</el-button>
      </el-upload>
      <span v-if="isDump">
        <el-input
          v-model="searchText"
          style="width: 240px; margin-left: 10px"
          @input="handleInputChange"
          placeholder="Search"
        />
        <el-button
          type="primary"
          @click="handleExport"
          style="margin-left: 10px"
          >Export to Json</el-button
        >
      </span>
    </div>
    <div class="content-bottom">
      <div class="content-bottom-item" style="width: 50%; height: 100%">
        <textarea
          ref="textarea"
          class="item-textarea"
          readonly
          rows="20"
          cols="80"
          >{{ dumpMsg }}</textarea
        >
      </div>
      <div style="width: 50%; padding: 20px; overflow: auto">
        <div v-for="(item, index) in searchList" :key="index">
          <div
            style="
              width: 100%;
              height: 30px;
              overflow: hidden;
              text-overflow: ellipsis;
              white-space: nowrap;
            "
            @click="jumpToKeyword(item.index)"
            v-html="highlight(item)"
          ></div>
        </div>
      </div>
    </div>
  </div>
</template>

<style scoped>
.content {
  width: 100%;
  height: 100%;
  background-color: #f9f9f9;
  box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1);
}

.content-top {
  width: 100%;
  height: 10%;
  display: flex;
  justify-content: center;
  align-items: center;
}

.content-bottom-item {
  display: flex;
  justify-content: center;
  align-items: center;
}

.file-input-label {
  display: inline-block;
  padding: 10px 20px;
  background-color: #007bff;
  color: white;
  font-size: 16px;
  font-weight: bold;
  border-radius: 5px;
  cursor: pointer;
}

.file-input-label {
  display: inline-block;
  padding: 10px 20px;
  background-color: #007bff;
  color: white;
  font-size: 16px;
  font-weight: bold;
  border-radius: 5px;
  cursor: pointer;
  transition: background-color 0.3s ease;
}

.file-input-label:hover {
  background-color: #0056b3;
}

.file-input-label input[type="file"] {
  display: none;
}

.content-bottom {
  width: 100%;
  height: 90%;
  display: flex;
  justify-content: space-around;
}

.auto-textarea {
  width: 100%;
  height: 90%;
  margin: 0;
  padding: 10px;
  border: 1px solid #ccc;
  border-radius: 5px;
  font-size: 15px;
  font-family: Consolas, monospace;
  overflow: auto;
}

.auto-textarea:focus {
  outline: none;
  border-color: #007bff;
  box-shadow: 0 0 5px rgba(0, 123, 255, 0.5);
}

.item-textarea {
  /* width: 100%; */
  margin: 0;
  /* padding: 10px; */
  width: 100%;
  height: 100%;
  padding: 10px;
  box-sizing: border-box;
  border: 1px solid #ccc;
  border-radius: 5px;
  font-size: 15px;
  font-family: Consolas, monospace;
  resize: none;
}

.dump-Item {
  width: 100%;
  height: 50pvh;
}
</style>
