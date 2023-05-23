#define USE_ORIG
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Text.RegularExpressions;

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
                    Path.GetFileName(path)
                },
                UseShellExecute = false,
                WorkingDirectory = Path.GetDirectoryName(path),
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

        private static Dictionary<string, string> ComputeTags()
        {
            var process = Process.Start(new ProcessStartInfo
            {
                FileName = LuaJitExePath,
                ArgumentList =
                {
                    Path.Combine(TestRunnerDirPath, "tags.lua")
                },
                UseShellExecute = false,
                RedirectStandardError = true,
                RedirectStandardOutput = true,
            })!;
            process.WaitForExit();
            var tags = new Dictionary<string, string>
            {
                { "slow", "true" }
            };
            var lineRegex = new Regex(@"^(?<K>[^=]+)=(?<V>.+)$");
            for (; ; )
            {
                var line = process.StandardOutput.ReadLine();
                if (line is null)
                    break;
                var match = lineRegex.Match(line);
                tags.Add(match.Groups["K"].Value, match.Groups["V"].Value);
            }
            return tags;
        }

        private static IEnumerable<TestCaseData> EnumerateTests()
        {
            using var reportWriter = File.CreateText(Path.Combine(TestRunnerDirPath, "discovery.txt"));

            var tags = ComputeTags();
            foreach (var kvp in tags)
                reportWriter.WriteLine($"Tag: {kvp.Key}={kvp.Value}");

            var tagRegex = new Regex(@"^(?<T>[+-])?(?<K>[^><=]+)(?<COMP>(?<OP>[><=]+)(?<V>[0-9]+(?:\.[0-9]+)?))?$");

            foreach (var data in EnumerateTests(TestsDirPath))
                yield return data;

            IEnumerable<TestCaseData> EnumerateTests(string dirPath)
            {
                var indexPath = Path.Combine(dirPath, "index");
                if (File.Exists(indexPath))
                {
                    foreach (var index in File.ReadAllLines(indexPath))
                    {
                        if (string.IsNullOrWhiteSpace(index))
                            continue;
                        var parts = index.Split(' ');

                        var name = parts[0];
                        var path = Path.Combine(dirPath, name);
                        
                        if (parts.Length > 1)
                        {
                            var match = tagRegex.Match(parts[1]);
                            var key = match.Groups["K"].Value;
                            var invert = match.Groups["T"].Success && match.Groups["T"].Value == "-";
                            bool present;
                            if (match.Groups["COMP"].Success)
                            {
                                var op = match.Groups["OP"].Value;
                                var comp = double.Parse(match.Groups["V"].Value, CultureInfo.InvariantCulture);
                                present =
                                    tags.TryGetValue(key, out var val) &&
                                    double.TryParse(val, NumberStyles.Float, CultureInfo.InvariantCulture, out var num) &&
                                    op switch
                                    {
                                        "<" => num < comp,
                                        ">" => num > comp,
                                        "<=" => num <= comp,
                                        ">=" => num >= comp,
                                        "==" => num == comp,
                                        _ => false
                                    };
                            }
                            else
                            {
                                present = tags.ContainsKey(key);
                            }
                            if (present == invert)
                            {
                                reportWriter.WriteLine($"Ignored test {path} {parts[1]}");
                                continue;
                            }
                            else
                            {
                                reportWriter.WriteLine($"Included test {path} {parts[1]}");
                            }
                        }
                        else
                            reportWriter.WriteLine($"Included test {path}");

                        if (name.EndsWith(".lua", StringComparison.OrdinalIgnoreCase))
                        {
                            var relPath = Path.GetRelativePath(TestsDirPath, path);
                            yield return new TestCaseData(Path.GetFullPath(path))
                                .SetName(Path.GetFileNameWithoutExtension(relPath))
                                .SetCategory(Path.GetDirectoryName(relPath)!)
                                .SetProperty("_CodeFilePath", path);
                        }
                        else
                        {
                            foreach (var data in EnumerateTests(path))
                                yield return data;
                        }
                    }
                }
                else
                {
                    foreach (var dir in Directory.GetDirectories(dirPath))
                        foreach (var data in EnumerateTests(dir))
                            yield return data;
                }
            }
        }
    }
}