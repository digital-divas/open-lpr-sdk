const { load, DataType, open } = require('node-ffi-rs');
const sharp = require('sharp');
const { readdir } = require('fs/promises');

const DET_SIZE = 36;

open({
    library: 'liblpr_sdk_shared',
    path: './build/liblpr_sdk_shared.dylib'
});

async function loadImageAsBGR(path) {
    const { data, info } = await sharp(path)
        .removeAlpha()           // garante 3 canais
        .raw()                   // formato raw (HWC)
        .toBuffer({ resolveWithObject: true });

    const width = info.width;
    const height = info.height;

    for (let i = 0; i < data.size; i += 3) {
        const r = data[i];
        data[i] = data[i + 2];   // B
        data[i + 2] = r;         // R
    }

    return {
        frameBuffer: data,
        width,
        height
    };
}

function parseResults(buffer, count) {
    const results = [];

    for (let i = 0; i < count; i++) {
        const offset = i * DET_SIZE;

        // plate (char[16])
        const plateBuf = buffer.subarray(offset, offset + 16);

        // remove \0 do final
        const plate = plateBuf.toString('utf8').replace(/\0/g, '');

        // confidence (float)
        const confidence = buffer.readFloatLE(offset + 16);

        const x1 = buffer.readInt32LE(offset + 20);
        const y1 = buffer.readInt32LE(offset + 24);
        const x2 = buffer.readInt32LE(offset + 28);
        const y2 = buffer.readInt32LE(offset + 32);

        results.push({
            plate,
            confidence,
            bbox: [x1, y1, x2, y2]
        });
    }

    return results;
}

/**
 * 
 * @param {boolean} verbose 
 * @returns 
 */
async function lpr_create(verbose) {
    return await load({
        library: "liblpr_sdk_shared",
        funcName: 'lpr_create',
        paramsType: [DataType.I32],
        paramsValue: [verbose ? 1 : 0],
        retType: DataType.External,
        runInNewThread: true,
    });
}

async function lpr_process(
    sdk,
    frameBuffer,
    width,
    height,
    resultsBuffer,
    maxResults,
) {
    const count = await load({
        library: "liblpr_sdk_shared",
        funcName: 'lpr_process',
        paramsType: [
            DataType.External,    // engine
            DataType.U8Array,     // frame
            DataType.I32,         // width
            DataType.I32,         // height
            DataType.U8Array,     // results pointer
            DataType.I32          // max_results

        ],
        paramsValue: [
            sdk,
            frameBuffer,
            width,
            height,
            resultsBuffer,
            maxResults,
        ],
        retType: DataType.I32,
        runInNewThread: true,
    });

    return parseResults(resultsBuffer, count);
}

async function main() {

    const sdk = await lpr_create(1);

    const images = await readdir('dataset');

    for (const image of images) {


        const maxResults = 10;
        const structSize = 36;

        const resultsBuffer = Buffer.alloc(structSize * maxResults);

        const { frameBuffer, width, height } = await loadImageAsBGR(`./dataset/${image}`);

        const now = new Date().getTime();

        const results = await lpr_process(
            sdk,
            frameBuffer,
            width,
            height,
            resultsBuffer,
            maxResults,
        );

        console.log(`dataset/${image} - process time: ${new Date().getTime() - now}ms`);

        for (const result of results) {
            console.log("Plate:", result.plate);
            console.log("Confidence:", result.confidence);
            console.log("BBox:", result.bbox);
        }
    }

}

main().then(() => process.exit(0)).catch(err => {
    console.error(err);
    process.exit(1);
});