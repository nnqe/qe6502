// single_step_sources.js

export const GITHUB_BASE_URLS_BY_MODEL = {
  mos: "https://raw.githubusercontent.com/SingleStepTests/65x02/main/6502/v1/",
  nes: "https://raw.githubusercontent.com/SingleStepTests/65x02/main/nes6502/v1/",
  wdc: "https://raw.githubusercontent.com/SingleStepTests/65x02/main/wdc65c02/v1/",
  rw: "https://raw.githubusercontent.com/SingleStepTests/65x02/main/rockwell65c02/v1/",
  st: "https://raw.githubusercontent.com/SingleStepTests/65x02/main/synertek65c02/v1/",
};

export function getGithubBaseUrlForModel(modelKey) {
  const url = GITHUB_BASE_URLS_BY_MODEL[modelKey];

  if (!url) {
    throw new Error(`No GitHub test URL configured for model: ${modelKey}`);
  }

  return url;
}

function hexOpcode(opcode) {
  return (opcode & 0xff).toString(16).padStart(2, "0").toLowerCase();
}

function parseOpcodeFromName(name) {
  const match = /(^|\/|\\)([0-9a-fA-F]{2})\.json$/u.exec(name);

  if (!match) {
    return null;
  }

  return Number.parseInt(match[2], 16);
}

function normalizeBaseUrl(baseUrl) {
  return baseUrl.endsWith("/") ? baseUrl : baseUrl + "/";
}

function makeLocalFileDescriptor(file, source) {
  const name = file.webkitRelativePath || file.name;
  const opcode = parseOpcodeFromName(name);

  if (opcode === null) {
    return null;
  }

  return {
    name,
    opcode,
    source,
    loadText: async () => file.text(),
  };
}

export function sourceFromLocalFile(file) {
  const descriptor = makeLocalFileDescriptor(file, "local-file");

  if (!descriptor) {
    throw new Error(`Local file is not an opcode JSON file: ${file.name}`);
  }

  return [descriptor];
}

export function sourceFromLocalFolder(fileList) {
  const files = Array.from(fileList);

  const descriptors = files
    .map((file) => makeLocalFileDescriptor(file, "local-folder"))
    .filter((descriptor) => descriptor !== null)
    .sort((a, b) => a.opcode - b.opcode || a.name.localeCompare(b.name));

  if (descriptors.length === 0) {
    throw new Error("No opcode JSON files found in selected folder");
  }

  return descriptors;
}

export function sourceFromGithubOpcode(baseUrl, opcode) {
  const normalizedBaseUrl = normalizeBaseUrl(baseUrl);
  const op = opcode & 0xff;
  const name = `${hexOpcode(op)}.json`;
  const url = normalizedBaseUrl + name;

  return [
    {
      name,
      opcode: op,
      source: "github-opcode",
      url,
      loadText: async () => {
        const response = await fetch(url);

        if (!response.ok) {
          throw new Error(
            `Failed to fetch ${url}: ${response.status} ${response.statusText}`
          );
        }

        return response.text();
      },
    },
  ];
}

export function sourceFromGithubFull(baseUrl) {
  const normalizedBaseUrl = normalizeBaseUrl(baseUrl);
  const descriptors = [];

  for (let opcode = 0; opcode <= 0xff; opcode++) {
    const name = `${hexOpcode(opcode)}.json`;
    const url = normalizedBaseUrl + name;

    descriptors.push({
      name,
      opcode,
      source: "github-full",
      url,
      loadText: async () => {
        const response = await fetch(url);

        if (!response.ok) {
          throw new Error(
            `Failed to fetch ${url}: ${response.status} ${response.statusText}`
          );
        }

        return response.text();
      },
    });
  }

  return descriptors;
}

export function parseOpcodeText(text) {
  const trimmed = text.trim();

  if (!/^[0-9a-fA-F]{1,2}$/u.test(trimmed)) {
    throw new Error(`Invalid opcode: ${text}`);
  }

  return Number.parseInt(trimmed, 16) & 0xff;
}
