val scala3Version = "3.2.1"

name := "tdjson-bind"

lazy val commonSettings = Seq(
  crossScalaVersions := Seq(scala3Version),
  scalaVersion := scala3Version,
  organization := "io.github.ablearthy",
  description := "Scala TDJson bindings",
  licenses := Seq("APL2" -> url("http://www.apache.org/licenses/LICENSE-2.0.txt")),
  homepage := Some(url("https://github.com/ablearthy/tdjson-bind")),
  versionScheme := Some("semver-spec"),
  version := "1.8.10-1",
  developers := List(
    Developer(
      "ablearthy",
      "Able Arthy",
      "ablearthy@gmail.com",
      url("https://github.com/ablearthy")
    )
  ),
  scmInfo := Some(
    ScmInfo(
      url("https://github.com/ablearthy/tdjson-bind"),
      "scm:git@github.com:ablearthy/tdjson-bind.git"
    )
  ),
  publish := {},
  publishLocal := {},
  pomIncludeRepository := { _ => false },
  publishTo := {
    val nexus = "https://s01.oss.sonatype.org/"
    if (isSnapshot.value) Some("snapshots" at nexus + "content/repositories/snapshots")
    else Some("releases" at nexus + "service/local/staging/deploy/maven2")
  },
  publishMavenStyle := true
)


lazy val root = project
  .in(file("."))
  .settings(commonSettings: _*)
  .settings(
    scalaVersion := scala3Version,
    crossScalaVersions := Nil
  )
  .aggregate(core, native)

lazy val core = project
  .in(file("core"))
  .settings(commonSettings: _*)
  .settings(
    javah / target := (native / nativeCompile / sourceDirectory).value / "include",

    name := "tdjson-bind-core",

    sbtJniCoreScope := Compile
  )
  .dependsOn(native % Runtime)

lazy val native = project
  .in(file("native"))
  .settings(commonSettings: _*)
  .settings(crossPaths := false)
  .settings(name := "tdjson-bind-native")
  .settings(nativeCompile / sourceDirectory := sourceDirectory.value)
  .settings(
    Compile / unmanagedPlatformDependentNativeDirectories := Seq(
      "x86_64-linux"  -> target.value / "native/x86_64-linux/bin/",
      "x86_64-darwin" -> target.value / "native/x86_64-darwin/bin/",
      "arm64-darwin"  -> target.value / "native/arm64-darwin/bin/"
    )
  )
  .settings(artifactName := { (sv: ScalaVersion, module: ModuleID, artifact: Artifact) => 
    artifact.name + "-" + nativePlatform.value + "-" + module.revision + "." + artifact.extension
  })
  .enablePlugins(JniNative, JniPackage)
