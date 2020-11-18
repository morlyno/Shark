
namespace Shark {

	__declspec(dllimport) void Print();

}

int main()
{
	Shark::Print();
	while ( true );
}