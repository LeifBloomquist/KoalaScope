namespace KoalaScopeServer
{
    internal class Program
    {
        static readonly CancellationTokenSource Cts = new();

        static async Task Main(string[] args)
        {
            Console.WriteLine("Starting KoalaScopeServer...");

            bool local = false;

            if (args.Length > 1)
            {
                if (args[1] == "local")
                {
                    Console.WriteLine("(Local)");
                    local = true;
                }
            }

            if (args.Length == 0)
            {
                Console.WriteLine("Need to supply path as first argument");
                return;
            }

            FileCollection fileCollection = new(args[0], "*" + Constants.KOA);
            new HttpServer().Start(local, fileCollection);

            await Task.Delay(Timeout.Infinite, Cts.Token);
        }
    }
}
