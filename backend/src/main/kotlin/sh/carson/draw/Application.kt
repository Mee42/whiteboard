package sh.carson.draw

import io.ktor.http.*
import io.ktor.server.application.*
import io.ktor.server.engine.*
import io.ktor.server.http.content.*
import io.ktor.server.netty.*
import io.ktor.server.request.*
import io.ktor.server.response.*
import io.ktor.server.routing.*
import sh.carson.draw.plugins.*
import java.io.File
import java.io.FileOutputStream

data class Point(val x: Int, val y: Int)

fun main() {
    embeddedServer(Netty, port = 8080, host = "0.0.0.0") {
        configureSerialization()
        configureRouting()
        routing {
            static("static/") {
                staticRootFolder = File("website/")
                files(".")
            }
            static("/") {
                staticRootFolder = File("website/")
                default("index.html")
            }
            post("print") {
                val points = call.receive<List<List<Int>>>()
                    .map { (a, b) -> Point(a, b) }
                printPicture(interpolate(points))
                call.respond(HttpStatusCode.OK)
            }
            post("instr") {
                sendToPrinter(call.receive())
                call.respond(HttpStatusCode.OK)
            }
        }
    }.start(wait = true)
}

fun interpolate(points: List<Point>): List<Point> {
    // we solve this by making a really fucking long list of points
    // and then remove duplicates
    // but we do this while streaming so it's ok :tm:
    // 100 points of interpolation between each one
    val pointCount = 100
    val morePoints = points
        .asSequence()
        .zipWithNext()
        .flatMap { (a, b) ->
            (0..pointCount).map { fraction ->
                val x = a.x + (b.x - a.x) * (fraction.toDouble() / pointCount)
                val y = a.y + (b.y - a.y) * (fraction.toDouble() / pointCount)
                Point(x.toInt(), y.toInt())
            }
        }
    val filteredPoints = mutableListOf<Point>(morePoints.first())
    for(elem in morePoints) {
        if(elem == filteredPoints.last()) continue
        filteredPoints.add(elem)
    }
    return filteredPoints
}

fun printPicture(points: List<Point>) {
    // we assume that the points start at 0
    // first we need to
    FileOutputStream(File("/dev/ttyACM0")).use { stream ->
        for(point in points) {
            Thread.sleep(100)
            stream.write("r${point.x},${point.y}\n".toByteArray(Charsets.US_ASCII))
            stream.flush()
        }
    }
}

// if instruction is bigger than the internal buffer
// expect sadness
fun sendToPrinter(instruction: String) {
    FileOutputStream(File("/dev/ttyACM0")).use { stream ->
        stream.write(instruction.toByteArray(Charsets.US_ASCII))
        stream.flush()
    }
}
