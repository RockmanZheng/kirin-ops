/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2025. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import { hilog } from '@kit.PerformanceAnalysisKit';
import testNapi from 'libentry.so';
import { image } from '@kit.ImageKit'
import { BusinessError } from '@kit.BasicServicesKit';
import fs from '@ohos.file.fs';
import { common } from '@kit.AbilityKit';
import hiai from 'libentry.so'

async function uint8ArrayToPixelMap(uint8Array: Uint8Array): Promise<image.PixelMap> {
  const buffer = uint8Array.buffer;
  const imageSource: image.ImageSource = image.createImageSource(buffer.slice(0))
  let decodingOptions: image.DecodingOptions = {
    editable: true,
    desiredPixelFormat: image.PixelMapFormat.RGBA_8888,
  };
  const pixelMap = imageSource.createPixelMapSync(decodingOptions);
  return pixelMap
}

function GetImageInfoSync(pixelMap:image.PixelMap):image.ImageInfo | undefined {
  if (pixelMap != undefined) {
    let imageInfo : image.ImageInfo = pixelMap.getImageInfoSync();
    return imageInfo;
  }
  return undefined ;
}

// 将灰度图转换为RGB888格式
async function grayToRGB888(grayData: Uint8Array, width: number, height: number): Promise<Uint8Array> {
  // 创建RGB888数据缓冲区（每个像素3字节）
  const rgbBuffer = new ArrayBuffer(width * height * 3);
  const rgbArray = new Uint8Array(rgbBuffer);

  // 遍历每个像素，复制灰度值到RGB三通道
  for (let i = 0; i < grayData.length; i++) {
    const grayValue = grayData[i];
    const baseIndex = i * 3;
    rgbArray[baseIndex] = grayValue;     // R通道
    rgbArray[baseIndex + 1] = grayValue; // G通道
    rgbArray[baseIndex + 2] = grayValue; // B通道
  }
  return rgbArray;
}

const VISIBLE:number = 0
const INVISIBLE:number = 2

const npuPreProcessImageWidth:number = 1024
const npuPreProcessImageHeight:number = 763

const npuPostProcessImageWidth:number = 1022
const npuPostProcessImageHeight:number = 761

@Entry
@Component
struct Index {
  @State processMessage: string = '';
  @State messageForNext : string = "Click for next image"
  @State index : number = 0
  @State pixelMap: image.PixelMap | null = null
  @State originalImage : Resource = $r('app.media.cup')
  @State originalVisible : Visibility = VISIBLE
  @State transformVisible : Visibility = INVISIBLE
  @State inferenceEnable : boolean = true
  @State imagesList : Resource[] = [$r('app.media.cup'), $r('app.media.guitar'), $r('app.media.airplane'),$r('app.media.wall_after'),$r('app.media.home_after')]

  saveImageToFile(data: ArrayBuffer, name: string) {
    const context = this.getUIContext().getHostContext() as common.UIAbilityContext;
    let filePath = context.tempDir + '/' + name;
    let file = fs.openSync(filePath, fs.OpenMode.READ_WRITE | fs.OpenMode.CREATE);
    try {

    } catch (error) {
      // TODO: Implement error handling.
    }
    try {
    } catch (error) {
      // TODO: Implement error handling.
    }
    fs.write(file.fd, data, (err, writeLen) => {
      if (err) {
        hilog.error(0x0000, 'CANNKitDemo', 'Write file failed!')
      } else {
        hilog.info(0x0000, 'CANNKitDemo', 'write data to file succeed and size is : %{public}d', writeLen)
      }
      fs.closeSync(file);
    });
  }

  build() {
    Row() {
      Column() {
        Text("CANN SobelFilter Demo")
          .fontSize(30)
          .fontWeight(FontWeight.Bold)
          .textAlign(TextAlign.Center)
          .margin({
          bottom:20
        })
       Row() {
        Image(this.originalImage)
          .onClick(() => {
            this.inferenceEnable = true
            this.processMessage = ""
          })
          .width('45%')
          .aspectRatio(1)
          .visibility(this.originalVisible)
          .margin({
          bottom : 20
        })

        Image(this.pixelMap)
          .onClick(() => {
            this.inferenceEnable = true
            this.originalVisible = VISIBLE
            this.transformVisible = INVISIBLE
            this.processMessage = ""
          })
          .width('45%')
          .aspectRatio(1)
          .visibility(this.transformVisible)
          .margin({
          bottom : 20
        })
      }.height('100%')
    }.height('100%').layoutWeight(2)

    Column() {
          Button('CPU推理')
            .enabled(this.inferenceEnable)
            .onClick(() => {
              this.pixelMap = null
              this.inferenceEnable = false
              const context = this.getUIContext().getHostContext() as common.UIAbilityContext;
              context.resourceManager.getMediaContent(this.originalImage.id, async (error: BusinessError, value: Uint8Array) => {
                  this.pixelMap = await uint8ArrayToPixelMap(value)
                  const buffer = new ArrayBuffer(this.pixelMap.getPixelBytesNumber());

                  await  this.pixelMap.readPixelsToBuffer(buffer).then(() => {
                    hilog.info(0x0000, 'CANNKitDemo', 'Succeeded in reading image pixel data')
                  }).catch((error : BusinessError) => {
                    hilog.error(0x0000, 'CANNKitDemo', 'Failed to read image pixel data. The error is : %{public}s', error)
                    this.inferenceEnable = true
                    return;
                  })

                  const info = GetImageInfoSync(this.pixelMap);
                  if (!info || info.size.width === undefined || info.size.height === undefined) {
                    hilog.error(0x0000, 'CANNKitDemo', "Get image info failed!")
                    this.inferenceEnable = true
                    return;
                  }
                  const width = GetImageInfoSync(this.pixelMap)?.size.width
                  const height =GetImageInfoSync(this.pixelMap)?.size.height

                  hilog.info(0x0000, 'CANNKitDemo', `width: ${width}, height: ${height}`)
                  let num:Uint8Array = testNapi.processImageWithSobel(new Uint8Array(buffer),width, height)
                  if (!num) {
                    hilog.error(0x0000, 'CANNKitDemo', 'SobelFilter operation failed!')
                    this.inferenceEnable = true
                    return;
                  }
                  // let pixmap = await image.createPixelMap(num.buffer.slice(0), {size: info.size, srcPixelFormat: info.pixelFormat});
                  let pixmap = await image.createPixelMap(num.buffer.slice(0), {size: info.size, srcPixelFormat: info.pixelFormat});
                  this.pixelMap = pixmap;
                  let runResult = testNapi.GetResult()
                  let cpuRunResult = "CPU运行时间 ： " + runResult[0] + "ms"
                  this.processMessage = cpuRunResult
                  this.transformVisible = VISIBLE
                  this.originalVisible = INVISIBLE

                }
                );
            })
            .margin(15)

          Button('NPU推理')
            .enabled(this.inferenceEnable)
            .onClick(() => {
              this.pixelMap = null
              this.inferenceEnable = false
              const context = this.getUIContext().getHostContext() as common.UIAbilityContext;
              context.resourceManager.getMediaContent(this.originalImage.id).then(value => {
                let buf = new ArrayBuffer(value.length)
                let tempBuffer = new Uint8Array(buf)

                for (let i = 0; i < value.length; i++) {
                  tempBuffer[i] = value[i]
                }
                const imageSource = image.createImageSource(buf)
                // Set the image format to BGRA_8888.
                imageSource.createPixelMap({desiredPixelFormat : image.PixelMapFormat.RGBA_8888})
                  .then(async pixelMapObj => {
                    let width = 0
                    let height = 0
                    await pixelMapObj.getImageInfo().then(async info => {
                      width = info.size.width
                      height = info.size.height
                    })
                    // Zoom the image to preprocess size
                    await pixelMapObj.scale(npuPreProcessImageWidth / width, npuPreProcessImageHeight / height)

                    const scaleBuffer = new ArrayBuffer(pixelMapObj.getPixelBytesNumber())
                    await   pixelMapObj.readPixelsToBuffer(scaleBuffer).then(() => {
                      hilog.info(0x0000, 'CANNKitDemo', 'Succeeded in reading image pixel data.')
                    }).catch((error : BusinessError) => {
                      hilog.error(0x0000, 'CANNKitDemo', 'Failed to read image pixel data. The error is : %{public}s', error)
                      this.inferenceEnable = true
                      return
                    })

                    pixelMapObj.getImageInfo().then(info => {
                      let resizeH = info.size.height
                      let resizeW = info.size.width

                      const readBuffer = new ArrayBuffer(pixelMapObj.getPixelBytesNumber())
                      pixelMapObj.readPixelsToBuffer(readBuffer, async (err, res)=> {
                        if (err) {
                          hilog.error(0x0000, 'CANNKitDemo', 'Failed to read image pixel data')
                          this.inferenceEnable = true
                          return
                        } else {
                          let dataArray = new Uint8Array(readBuffer)
                          let len1 = npuPreProcessImageHeight
                          let len2 = npuPreProcessImageWidth
                          let len3 = 4
                          let pixelArray : Array<Array<Uint8Array>> = new Array<Array<Uint8Array>>(len1)
                          for (let i = 0; i < len1; ++i) {
                            pixelArray[i] = new Array<Uint8Array>(len2)
                            for (let j = 0; j < len2; ++j) {
                              pixelArray[i][j] = new Uint8Array(len3)
                              for (let k = 0; k < len3; ++k) {
                                pixelArray[i][j][k] = dataArray[i * len2 * len3 + j * len3 + k]
                              }
                            }
                          }
                          // Sets the input BGR channel value.
                          let channel = 3;
                          const color = new ArrayBuffer(channel * resizeW * resizeH);
                          let inputData = new Uint8Array(color);
                          for (let i = 0; i < resizeW; i++) {
                            for (let j = 0; j < resizeH; j++) {
                              // NHWC 排列，每个像素紧密存储3个通道
                              const baseIndex = (j * resizeW + i) * channel;
                              inputData[baseIndex + 0] = pixelArray[j][i][0]; // R
                              inputData[baseIndex + 1] = pixelArray[j][i][1]; // G
                              inputData[baseIndex + 2] = pixelArray[j][i][2]; // B
                            }
                          }

                          // Initialize input and output data.
                          let ret = testNapi.InitIOTensors(inputData)
                          if (ret != 0) {
                            hilog.error(0x0000, 'CANNKitDemo', 'Failed to init IOTensors.')
                            this.inferenceEnable = true
                            return
                          }

                          // Model Inference
                          ret = testNapi.RunModel()
                          if (ret != 0) {
                            hilog.error(0x0000, 'CANNKitDemo', 'Failed to run model.')
                            this.inferenceEnable = true
                            return
                          }

                          // Get model Output
                          let runResult = testNapi.GetResult()

                          let npuRunResult = "NPU运行时间 ： " + runResult[1] + "ms"
                          this.processMessage = npuRunResult

                          let filterResult:Uint8Array = testNapi.GetSobelResult()
                          filterResult = await grayToRGB888(filterResult, npuPostProcessImageHeight, npuPostProcessImageWidth);
                          hilog.info(0x0000, 'CANNKitDemo', 'Result length is %{public}d.', filterResult.byteLength)

                          this.processMessage = npuRunResult
                          this.transformVisible = VISIBLE
                          this.originalVisible = INVISIBLE
                          hilog.info(0x0000, 'HiAIFoundationDemo', 'info.pixelFormat is %{public}d.', info.pixelFormat);
                          let pixmap = await image.createPixelMap(filterResult.buffer.slice(0), {size: { height : npuPostProcessImageHeight, width :npuPostProcessImageWidth}, srcPixelFormat: image.PixelMapFormat.RGB_888});
                          await pixmap.scale(width / 1022, height / 761)
                          this.pixelMap = pixmap;
                        }
                      })
                    })
                  })
              })
            })
            .margin(15)

          Button(this.messageForNext)
            .enabled(this.inferenceEnable)
            .onClick(() => {
              this.index++;
              this.pixelMap = null
              if (this.index >= this.imagesList.length) {
                this.index = 0
              }
              this.originalImage = this.imagesList[this.index]
              this.pixelMap = null
            })
            .margin(15)

          Text(this.processMessage)
          .textAlign(TextAlign.Center)
          .fontSize(17)
          .fontColor(0xFF69B4)
          .padding('2%')
          .width('100%')
          .height('5%')
          .backgroundColor(Color.White)
        }.height('match_parent').layoutWeight(1)
      }.width('match_parent').height('match_parenet')
    }
}

