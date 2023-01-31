package io.github.ablearthy.tdjson

import com.github.sbt.jni.syntax.NativeLoader

trait LogMessageHandler {
  def onMessage(verbosityLevel: Int, message: String): Unit
}

class TDJson extends NativeLoader("tdjson_jni.1.8.10") {
  @native def td_create_client_id: Int
  @native def td_receive(timeout: Double): String
  @native def td_send(clientId: Int, request: String): Unit
  @native def td_execute(request: String): String
  @native def td_set_log_message_callback(maxVerbosityLevel: Int, handler: LogMessageHandler): Unit
}