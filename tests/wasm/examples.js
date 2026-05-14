const EXAMPLES = {
  hello_world: {
    title: "Hello World",
    module: "./example_hello_world.js",
  },

  klaus2m5: {
    title: "Klaus2m5",
    module: "./example_klaus2m5.js",
  },

  print_meta: {
    title: "Opcodes metadata",
    module: "./example_print_meta.js",
  },
  serialize: {
    title: "Serialize CPU",
    module: "./example_serialize.js",
  },
  single_step_core_test: {
    title: "Single Test Core Test",
    module: "./example_single_step_core_test.js",
  },
  single_step: {
    title: "Single Step Test",
    module: "./example_single_step.js",
  },
};

const params = new URLSearchParams(window.location.search);
const exampleId = params.get("example");

const home = document.getElementById("home");
const runner = document.getElementById("runner");
const title = document.getElementById("example-title");
const output = document.getElementById("output");

if (!exampleId) {
  home.classList.remove("hidden");
  runner.classList.add("hidden");
} else {
  const example = EXAMPLES[exampleId];

  home.classList.add("hidden");
  runner.classList.remove("hidden");

  if (!example) {
    title.textContent = "Unknown example";
    output.textContent = `Unknown example: ${exampleId}`;
  } else {
    title.textContent = example.title;
    output.textContent = "Loading example...\n";

    try {
      const mod = await import(example.module);

      if (typeof mod.run !== "function") {
        throw new Error(`${example.module} does not export run()`);
      }

      output.textContent = "";
      await mod.run({ output });
    } catch (error) {
      output.textContent += "\nERROR:\n";
      output.textContent +=
        error instanceof Error ? error.stack : String(error);
    }
  }
}
