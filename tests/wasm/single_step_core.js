// single_step_core.js

function normalizeP(p) {
  return (p | 0x30) & 0xff;
}

function hex8(value) {
  return "0x" + (value & 0xff).toString(16).padStart(2, "0").toUpperCase();
}

function hex16(value) {
  return "0x" + (value & 0xffff).toString(16).padStart(4, "0").toUpperCase();
}

function makeFailure(name, reason, extra = {}) {
  return {
    passed: false,
    name,
    reason,
    ...extra,
  };
}

function applyRam(memory, ramEntries) {
  for (const [address, value] of ramEntries) {
    memory[address & 0xffff] = value & 0xff;
  }
}

function getCycleType(cpu) {
  return cpu.hasData() ? "write" : "read";
}

function readActualCycleData(cpu, memory, address, type) {
  if (type === "read") {
    return memory[address];
  }

  return cpu.readData();
}

function serviceCycle(cpu, memory, address, type, data) {
  if (type === "read") {
    cpu.feedData(data);
  } else {
    memory[address] = data;
  }

  cpu.tick();
}

function compareRegister(name, actual, expected, testName) {
  if (actual !== expected) {
    return makeFailure(testName, "register mismatch", {
      register: name,
      expected,
      actual,
      expectedHex: name === "pc" ? hex16(expected) : hex8(expected),
      actualHex: name === "pc" ? hex16(actual) : hex8(actual),
    });
  }

  return null;
}

export function runSingleStepCase(qe, model, testCase) {
  const testName = testCase.name ?? "<unnamed>";

  const memory = new Uint8Array(0x10000);
  applyRam(memory, testCase.initial.ram);

  const cpu = qe.createCPU();

  try {
    cpu.powerOn(model);

    cpu.overwriteRegs({
      pc: testCase.initial.pc,
      a: testCase.initial.a,
      x: testCase.initial.x,
      y: testCase.initial.y,
      s: testCase.initial.s,
      p: normalizeP(testCase.initial.p),
      opcode: 0,
    });

    for (
      let cycleIndex = 0;
      cycleIndex < testCase.cycles.length;
      cycleIndex++
    ) {
      const [expectedAddressRaw, expectedDataRaw, expectedType] =
        testCase.cycles[cycleIndex];

      const expectedAddress = expectedAddressRaw & 0xffff;
      const expectedData = expectedDataRaw & 0xff;

      const actualAddress = cpu.address();
      const actualType = getCycleType(cpu);
      const actualData = readActualCycleData(
        cpu,
        memory,
        actualAddress,
        actualType,
      );

      if (actualAddress !== expectedAddress) {
        return makeFailure(testName, "cycle address mismatch", {
          cycleIndex,
          expected: {
            address: expectedAddress,
            addressHex: hex16(expectedAddress),
            data: expectedData,
            dataHex: hex8(expectedData),
            type: expectedType,
          },
          actual: {
            address: actualAddress,
            addressHex: hex16(actualAddress),
            data: actualData,
            dataHex: hex8(actualData),
            type: actualType,
          },
        });
      }

      if (actualType !== expectedType) {
        return makeFailure(testName, "cycle type mismatch", {
          cycleIndex,
          expected: {
            address: expectedAddress,
            addressHex: hex16(expectedAddress),
            data: expectedData,
            dataHex: hex8(expectedData),
            type: expectedType,
          },
          actual: {
            address: actualAddress,
            addressHex: hex16(actualAddress),
            data: actualData,
            dataHex: hex8(actualData),
            type: actualType,
          },
        });
      }

      if (actualData !== expectedData) {
        return makeFailure(testName, "cycle data mismatch", {
          cycleIndex,
          expected: {
            address: expectedAddress,
            addressHex: hex16(expectedAddress),
            data: expectedData,
            dataHex: hex8(expectedData),
            type: expectedType,
          },
          actual: {
            address: actualAddress,
            addressHex: hex16(actualAddress),
            data: actualData,
            dataHex: hex8(actualData),
            type: actualType,
          },
        });
      }

      serviceCycle(cpu, memory, actualAddress, actualType, actualData);
    }

    if (!cpu.isInstrDone()) {
      return makeFailure(
        testName,
        "instruction not completed after expected cycles",
        {
          regs: cpu.readRegs(),
        },
      );
    }

    if (!cpu.ok()) {
      return makeFailure(testName, "cpu not ok", {
        errorCode: cpu.errorCode(),
        regs: cpu.readRegs(),
      });
    }

    const regs = cpu.readRegs();

    const expectedFinal = {
      pc: testCase.final.pc & 0xffff,
      a: testCase.final.a & 0xff,
      x: testCase.final.x & 0xff,
      y: testCase.final.y & 0xff,
      s: testCase.final.s & 0xff,
      p: normalizeP(testCase.final.p),
    };

    const actualFinal = {
      pc: regs.pc & 0xffff,
      a: regs.a & 0xff,
      x: regs.x & 0xff,
      y: regs.y & 0xff,
      s: regs.s & 0xff,
      p: normalizeP(regs.p),
    };

    for (const registerName of ["pc", "a", "x", "y", "s", "p"]) {
      const failure = compareRegister(
        registerName,
        actualFinal[registerName],
        expectedFinal[registerName],
        testName,
      );

      if (failure) {
        failure.expectedRegs = expectedFinal;
        failure.actualRegs = actualFinal;
        return failure;
      }
    }

    for (const [addressRaw, expectedValueRaw] of testCase.final.ram) {
      const address = addressRaw & 0xffff;
      const expectedValue = expectedValueRaw & 0xff;
      const actualValue = memory[address];

      if (actualValue !== expectedValue) {
        return makeFailure(testName, "final ram mismatch", {
          address,
          addressHex: hex16(address),
          expected: expectedValue,
          expectedHex: hex8(expectedValue),
          actual: actualValue,
          actualHex: hex8(actualValue),
          regs: actualFinal,
        });
      }
    }

    return {
      passed: true,
      name: testName,
      cycles: testCase.cycles.length,
      regs: actualFinal,
    };
  } finally {
    cpu.dispose();
  }
}
