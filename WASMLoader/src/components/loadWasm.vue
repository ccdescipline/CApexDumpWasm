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
const version = ref("")


onMounted(async () => {
  console.log("hello");

  // const moduleInstance = await MainModule({ locateFile: (file) => myWasmUrl });
  // console.log(moduleInstance); // 输出 5



  CmakeModuleInstanse = await DumpWasmModule();
  let ressult = CmakeModuleInstanse._add(2, 3);
  console.log(ressult); // 输出 5
  console.log(CmakeModuleInstanse);

  const vPtr = CmakeModuleInstanse._getVersion();
  version.value = CmakeModuleInstanse.UTF8ToString(vPtr);


  worker = new Worker(new URL('../wasm/wasmWorker.js', import.meta.url), { type: 'module' });

   worker.onmessage = (e) => {
    const { type, parsetJson, errorJson } = e.data;
    if (type === 'runHeavy') {
      dumpObj.value = parsetJson;
      console.log("worker onmessage")

      console.log(parsetJson);
      console.log("errorJson", errorJson);
      formatDump();

      const totalErrors = errorJson
        ? Object.values(errorJson).reduce((n, arr) => n + (arr?.length || 0), 0)
        : 0;
      if (totalErrors > 0) {
        ElMessage.warning(`Dump finished with ${totalErrors} warning${totalErrors === 1 ? '' : 's'}. Press F12 to see details in the console.`);
      }

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

  let currentSection = '';
  const lines = dumpMsg.value.split("\n");
  const results = [];

  for (let i = 0; i < lines.length; i++) {
    const line = lines[i];
    const sectionMatch = line.match(/^\[(.+?)\]\s*$/);
    if (sectionMatch) {
      currentSection = sectionMatch[1];
    } else if (line.toLowerCase().includes(value.toLowerCase())) {
      results.push({ index: i, text: line, section: currentSection });
    }
  }

  searchList.value = results;
}

function highlight(item) {
  const escapeRegExp = (string) => string.replace(/[.*+?^${}()|[\]\\]/g, "\\$&");
  if (!searchText.value) return item.text;
  const escaped = escapeRegExp(searchText.value);
  const regex = new RegExp(`(${escaped})`, "gi");
  return item.text.replace(regex, `<mark class="hl">$1</mark>`);
}

function jumpToKeyword(line) {
  // Scroll the textarea to the target position
  //textarea.scrollTop = scrollPosition;
  console.log(textarea.value);
  textarea.value.scrollTop = line * 18 + 10;
}

const copyLabel = ref("Copy");

const handleCopy = async function () {
  await navigator.clipboard.writeText(dumpMsg.value);
  copyLabel.value = "Copied!";
  setTimeout(() => { copyLabel.value = "Copy"; }, 2000);
};

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
      <div class="top-right">
        <span v-if="version" class="version-badge">v{{ version }}</span>
      <a
        class="github-link"
        href="https://github.com/ccdescipline/CApexDumpWasm"
        target="_blank"
        rel="noopener noreferrer"
      >
        <svg height="18" width="18" viewBox="0 0 16 16" fill="currentColor" aria-hidden="true">
          <path d="M8 0C3.58 0 0 3.58 0 8c0 3.54 2.29 6.53 5.47 7.59.4.07.55-.17.55-.38
            0-.19-.01-.82-.01-1.49-2.01.37-2.53-.49-2.69-.94-.09-.23-.48-.94-.82-1.13
            -.28-.15-.68-.52-.01-.53.63-.01 1.08.58 1.23.82.72 1.21 1.87.87
            2.33.66.07-.52.28-.87.51-1.07-1.78-.2-3.64-.89-3.64-3.95
            0-.87.31-1.59.82-2.15-.08-.2-.36-1.02.08-2.12 0 0 .67-.21 2.2.82
            .64-.18 1.32-.27 2-.27s1.36.09 2 .27c1.53-1.04 2.2-.82 2.2-.82
            .44 1.1.16 1.92.08 2.12.51.56.82 1.27.82 2.15 0 3.07-1.87 3.75-3.65
            3.95.29.25.54.73.54 1.48 0 1.07-.01 1.93-.01 2.2 0 .21.15.46.55.38
            A8.013 8.013 0 0 0 16 8c0-4.42-3.58-8-8-8z"/>
        </svg>
        CApexDumpWasm
      </a>
      </div>
    </div>
    <div class="content-bottom">
      <div class="content-bottom-item" style="width: 50%; height: 100%; position: relative;">
        <textarea
          ref="textarea"
          class="item-textarea"
          readonly
          rows="20"
          cols="80"
          >{{ dumpMsg }}</textarea
        >
        <el-button
          v-if="isDump"
          class="copy-btn"
          size="small"
          @click="handleCopy"
        >{{ copyLabel }}</el-button>
      </div>
      <div class="search-panel">
        <template v-if="searchText">
          <div class="result-count" v-if="searchList.length > 0">
            {{ searchList.length }} result{{ searchList.length === 1 ? '' : 's' }} found
          </div>
          <div class="result-empty" v-else>No results found</div>
        </template>
        <div
          v-for="(item, index) in searchList"
          :key="index"
          class="result-card"
          @click="jumpToKeyword(item.index)"
        >
          <span class="result-section" v-if="item.section">[{{ item.section }}]</span>
          <span v-html="highlight(item)"></span>
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
  position: relative;
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

.copy-btn {
  position: absolute;
  top: 8px;
  right: 28px;
  opacity: 0.7;
  transition: opacity 0.2s;
}

.copy-btn:hover {
  opacity: 1;
}

.dump-Item {
  width: 100%;
  height: 50pvh;
}

.top-right {
  position: absolute;
  right: 16px;
  display: flex;
  align-items: center;
  gap: 10px;
}

.version-badge {
  font-size: 12px;
  color: #909399;
  background: #f0f0f0;
  border-radius: 10px;
  padding: 2px 8px;
  font-family: Consolas, monospace;
}

.github-link {
  display: flex;
  align-items: center;
  gap: 6px;
  color: #333;
  text-decoration: none;
  font-size: 14px;
  font-weight: 500;
  transition: color 0.2s;
  position: static;
}

.github-link:hover {
  color: #409eff;
}

.search-panel {
  width: 50%;
  height: 100%;
  padding: 16px;
  box-sizing: border-box;
  overflow-y: auto;
  display: flex;
  flex-direction: column;
  gap: 6px;
  align-content: flex-start;
}

.result-count {
  font-size: 13px;
  color: #909399;
  padding: 2px 4px;
  margin-bottom: 4px;
}

.result-empty {
  font-size: 13px;
  color: #c0c4cc;
  padding: 12px 4px;
  text-align: center;
}

.result-card {
  flex-shrink: 0;
  padding: 6px 10px;
  border: 1px solid #e0e0e0;
  border-radius: 5px;
  background: #fff;
  cursor: pointer;
  font-family: Consolas, monospace;
  font-size: 13px;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
  transition: background-color 0.15s, border-color 0.15s;
}

.result-card:hover {
  background-color: #ecf5ff;
  border-color: #409eff;
}

.result-section {
  font-size: 11px;
  color: #909399;
  margin-right: 6px;
  flex-shrink: 0;
}

:deep(.hl) {
  background-color: #ffe566;
  color: inherit;
  border-radius: 2px;
  padding: 0 1px;
}
</style>
