using System.Text.RegularExpressions;

namespace KoalaChecker
{
    internal class Program
    {
        public static string Sanitize(string input)
        {
            return Regex.Replace(input, @"[^a-zA-Z0-9.]", "_");
        }

        static void Main(string[] args)
        {
            if (args.Length != 2) return;

            string directory = args[0];
            string buggy = args[1];

            byte[] valid_addresses = [0x44, 0x60, 0x20, 0x40, 00];

            Console.WriteLine("Directory = " + directory);

            string[] files = Directory.GetFiles(directory, "*.koa");            

            foreach (string orig_file in files)
            {

                bool ok = true;

                Console.Write("Checking [" + orig_file + "]: ");

                string orig_name_nopath = Path.GetFileName(orig_file);

                // Fix Filename if needed
                string filename = Sanitize(orig_name_nopath);
                File.Move(directory + orig_name_nopath, directory + filename, true);
                string file = directory + filename;

                byte[] image = File.ReadAllBytes(file);

                // Size
                if (image.Length != 10003)
                {
                    Console.Write("File size mismatch:" + image.Length + "   ");
                    ok = false;
                }

                // Load Address - Low Byte 
                if (image[0] != 0)
                {
                    Console.Write("Invalid start byte:" + image[0] + "   ");
                    ok = false;
                }

                // Load Address - High Byte
                if (!valid_addresses.Contains(image[1]))
                {
                    Console.Write("Invalid load address:" + image[1] + "   ");
                    ok = false;
                }

                if (ok)
                {
                    Console.WriteLine("OK");
                }
                else
                {
                    Console.WriteLine("ERROR");
                    File.Move(file, buggy + Path.GetFileName(file), true);
                }
            }

            Console.WriteLine("Done");
            Console.ReadLine();
        }
    }
}
