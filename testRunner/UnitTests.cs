//#define USE_ORIG
using NUnit.Framework.Interfaces;
using NUnit.Framework.Internal;
using System.Diagnostics;

namespace tests
{
    public class Tests
    {
#if USE_ORIG
        private const string LuaJitDirPath = "bin\\x64\\Orig";
#elif DEBUG
        private const string LuaJitDirPath = "bin\\x64\\Debug";
#else
        private const string LuaJitDirPath = "bin\\x64\\Release";
#endif
        private const string RunnerFlags = "-none";

        private static string SolutionDirPath => Path.GetFullPath(Path.Combine(Environment.CurrentDirectory, "..\\..\\..\\..\\"));

        private static string TestsDirPath => Path.Combine(SolutionDirPath, "tests");
        private static string LuaJitExePath => Path.Combine(SolutionDirPath, LuaJitDirPath, "luajit.exe");
        private static string TestRunnerDirPath => Path.Combine(SolutionDirPath, "testRunner");

        [TestCaseSource(nameof(EnumerateTests))]
        public void RunTest(string path)
        {
            var luaJitExePath = LuaJitExePath;
            var luaJitExePathLinePrefix = luaJitExePath + ": ";

            Console.WriteLine();
            Console.WriteLine("Using " + luaJitExePath);

            string ProcessOutputLine(string s)
            {
                if (s.StartsWith(luaJitExePathLinePrefix, StringComparison.OrdinalIgnoreCase))
                    s = s[luaJitExePathLinePrefix.Length..];
                return s;
            }

            async Task ConsumeStreamAsync(StreamReader reader)
            {
                for (; ; )
                {
                    var line = await reader.ReadLineAsync();
                    if (line is null)
                        break;
                    if (line.StartsWith("seed: "))
                        continue;
                    Console.WriteLine(ProcessOutputLine(line));
                }
            }

            var process = Process.Start(new ProcessStartInfo
            {
                FileName = luaJitExePath,
                ArgumentList =
                {
                    Path.Combine(TestsDirPath, "test", "test.lua"),
                    RunnerFlags,
                    path
                },
                UseShellExecute = false,
                WorkingDirectory = Path.Combine(TestsDirPath, "test"),
                RedirectStandardError = true,
                RedirectStandardOutput = true,
            })!;

            var stderrTask = ConsumeStreamAsync(process.StandardError);
            var stdoutTask = ConsumeStreamAsync(process.StandardOutput);

            process.WaitForExit();
            stderrTask.Wait();
            stdoutTask.Wait();

            if (process.ExitCode != 0)
            {
                Assert.Fail("Test failed");
            }
        }

        private static IEnumerable<TestCaseData> EnumerateTests()
        {
            var process = Process.Start(new ProcessStartInfo
            {
                FileName = LuaJitExePath,
                ArgumentList =
                {
                    Path.Combine(TestsDirPath, "test", "test.lua"),
                    RunnerFlags,
                    "--list"
                },
                UseShellExecute = false,
                RedirectStandardError = true,
                RedirectStandardOutput = true,
            })!;

            for (; ; )
            {
                var line = process.StandardOutput.ReadLine();
                if (line is null)
                    break;

                var path = line;
                var skipped = path.StartsWith("-");
                var skipReason = string.Empty;
                if (skipped)
                {
                    var sep = line.IndexOf(' ');
                    path = line[1..sep];
                    skipReason = line[(sep + 1)..];
                }

                var relPath = Path.GetRelativePath(TestsDirPath, path);
                yield return new TestCaseData(Path.GetFullPath(path))
                {
                    RunState = skipped ? RunState.Ignored : RunState.Runnable,
                }
                .SetName(Path.GetFileNameWithoutExtension(relPath))
                .SetCategory(Path.GetDirectoryName(relPath)!)
                .SetProperty("_CodeFilePath", path)
                .SetProperty(PropertyNames.SkipReason, skipReason);
            }

            var error = process.StandardError.ReadToEnd();
            if (!string.IsNullOrWhiteSpace(error))
                throw new InvalidOperationException(error);

            process.WaitForExit();
        }
    }
}