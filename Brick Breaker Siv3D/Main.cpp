# include <Siv3D.hpp>

/*
	よりC++ライクな書き方
	・クラスベース
	・継承を行う
*/

//==============================
// 前方宣言
//==============================
class Ball;
class Bricks;
class Paddle;

//==============================
// 定数
//==============================
namespace constants {
	namespace brick {
		/// @brief ブロックのサイズ
		constexpr Size SIZE{ 40, 20 };

		/// @brief ブロックの数　縦
		constexpr int Y_COUNT = 5;

		/// @brief ブロックの数　横
		constexpr int X_COUNT = 20;

		/// @brief 合計ブロック数
		constexpr int MAX = Y_COUNT * X_COUNT;
	}

	namespace ball {
		/// @brief ボールの速さ
		constexpr double SPEED = 480.0;
	}

	namespace paddle {
		/// @brief パドルのサイズ
		constexpr Size SIZE{ 60, 10 };
	}

	namespace reflect {
		/// @brief 縦方向ベクトル
		constexpr Vec2 VERTICAL		{  1, -1 };

		/// @brief 横方向ベクトル
		constexpr Vec2 HORIZONTAL	{ -1,  1 };
	}
}

//==============================
// 列挙
//==============================
namespace brick{
	namespace intersect{
		enum class Result {
			NONE,		// 衝突なし
			VERTICAL,	// 縦向きの衝突
			HORIZONTAL	// 横向きの衝突
		};
	};
	
};

//==============================
// クラス宣言
//==============================
#pragma region インターフェース
class IObject
{
public:
	virtual ~IObject() {}

public:
	virtual void Draw() const = 0;
};

class IGameScene {
public:
	virtual ~IGameScene() {}

public:
	virtual void Update() = 0;
	virtual void Collision() = 0;
	virtual void Draw() = 0;
};
#pragma endregion

/// @brief ボール
class Ball final : public IObject {
private:
	/// @brief 速度
	Vec2 velocity;

	/// @brief ボール
	Circle ball;

public:
	/// @brief コンストラクタ
	Ball() : velocity({ 0, -constants::ball::SPEED }), ball({ 400, 400, 8 }) {}

	/// @brief デストラクタ
	~Ball() {}

	/// @brief 更新
	void Update() {
		ball.moveBy(velocity * Scene::DeltaTime());
	}

	Circle GetCircle() const { return ball; }

	Vec2 GetVelocity() const { return velocity; }

	/// @brief 新しい移動速度を設定
	/// @param newVelocity 新しい移動速度
	void SetVelocity( Vec2 newVelocity ) {
		using namespace constants::ball;
		velocity = newVelocity.setLength( SPEED );
	}

	/// @brief 反射
	/// @param reflectVec 反射ベクトル方向 
	void Reflect( const Vec2 reflectVec ) {
		velocity *= reflectVec;
	}

public:
	/// @brief 描画
	void Draw() const final { ball.draw(); }
};

/// @brief ブロック
class Brick final : public IObject {
private:
	Rect _brick;

	int _life;

public:
	Brick() : _life(0), _brick({}) {}
	Brick(int x, int y, int life) : _life( life ) {
		using namespace constants::brick;
		_brick = Rect{
			x * SIZE.x,
			60 + y * SIZE.y,
			SIZE
		};
	}
	~Brick() {}

	brick::intersect::Result Intersects(Ball* const target);

public:
	void Draw() const final {
		if( _life > 0 ) _brick.stretched(-1).draw(HSV{ _brick.y - 40 });
	}
};

/// @brief ブロック
class Bricks final {
private:
	/// @brief ブロックリスト
	Brick _brickTable[constants::brick::MAX];

public:
	/// @brief コンストラクタ
	Bricks() {
		using namespace constants::brick;

		for (int y = 0; y < Y_COUNT; ++y) {
			for (int x = 0; x < X_COUNT; ++x) {
				int index = y * X_COUNT + x;
				_brickTable[index] = Brick{ x, y, 1 };
			}
		}
	}

	/// @brief デストラクタ
	~Bricks() {}

	/// @brief 衝突検知
	void Intersects(Ball* const target);

	/// @brief 描画
	void Draw() const {
		using namespace constants::brick;

		for (int i = 0; i < MAX; ++i) {
			_brickTable[i].Draw();
		}
	}
};

/// @brief パドル
class Paddle final : public IObject {
private:
	Rect paddle;

public:
	/// @brief コンストラクタ
	Paddle() : paddle(Rect(Arg::center(Cursor::Pos().x, 500), constants::paddle::SIZE)) {}

	/// @brief デストラクタ
	~Paddle() {}

	/// @brief 衝突検知
	void Intersects(Ball* const target) const;

	/// @brief 更新
	void Update() {
		paddle.x = Cursor::Pos().x - (constants::paddle::SIZE.x / 2);
	}

public:
	/// @brief 描画
	void Draw() const final { paddle.rounded(3).draw(); }
};

/// @brief 壁
class Field {
public:
	/// @brief 衝突検知
	static void Intersects(Ball* target) {
		using namespace constants;

		if ( !target ) {
			return;
		}

		auto velocity = target->GetVelocity();
		auto ball = target->GetCircle();

		// 天井との衝突を検知
		if ((ball.y < 0) && (velocity.y < 0))
		{
			target->Reflect( reflect::VERTICAL );
		}

		// 壁との衝突を検知
		if (((ball.x < 0) && (velocity.x < 0))
			|| ((Scene::Width() < ball.x) && (0 < velocity.x)))
		{
			target->Reflect( reflect::HORIZONTAL );
		}
	}

	/// @brief 画面外に出ているか
	/// @param target 対象オブジェクト
	/// @return 出ている場合はtrue
	static bool IsOutScreen( Ball* target ) {
		if (!target) {
			return false;
		}

		auto ball = target->GetCircle();
		return (Scene::Height() > ball.y);
	}
};

class GameMain : public IGameScene {
public:
	GameMain() {
	}

	virtual ~GameMain() {}

public:
	void Update() final {}
	void Collision() final {}
	void Draw() final {}
};

//==============================
// クラスメンバ関数定義
//==============================
#pragma region Brick
brick::intersect::Result Brick::Intersects(Ball* const target) {
	using namespace brick::intersect;

	if ( _life <= 0 ) {
		return Result::NONE;
	}

	auto ball = target->GetCircle();

	if ( _brick.intersects( ball ) ) {
		_life--;

		// ブロックの上辺、または底辺と交差
		if (_brick.bottom().intersects(ball)
			|| _brick.top().intersects(ball))
		{
			return Result::VERTICAL;
		}
		else // ブロックの左辺または右辺と交差
		{
			return Result::HORIZONTAL;
		}
	}
	return Result::NONE;
}
#pragma endregion

#pragma region Bricks
void Bricks::Intersects(Ball* const target) {
	using namespace constants::brick;
	using namespace brick::intersect;

	if (!target) {
		return;
	}

	for (int i = 0; i < MAX; ++i) {
		Brick& refBrick( _brickTable[i] );

		switch ( refBrick.Intersects(target) )
		{
		case Result::VERTICAL:
			target->Reflect(constants::reflect::VERTICAL);
			return;

		case Result::HORIZONTAL:
			target->Reflect(constants::reflect::HORIZONTAL);
			return;

		case Result::NONE:
		default:
			break;
		}
	}
}
#pragma endregion

#pragma region Paddle
void Paddle::Intersects(Ball* const target) const {
	if (!target) {
		return;
	}

	auto velocity = target->GetVelocity();
	auto ball = target->GetCircle();

	// ベクトルが下方向の時のみ反射
	if ((0 < velocity.y) && paddle.intersects(ball))
	{
		target->SetVelocity(Vec2{
			(ball.x - paddle.center().x) * 10,
			-velocity.y
		});
	}
}
#pragma endregion

//==============================
// エントリー
//==============================
void Main()
{
	Bricks bricks;
	Ball ball;
	Paddle paddle;

	while (System::Update())
	{
		//==============================
		// 更新
		//==============================
		paddle.Update();
		ball.Update();

		//==============================
		// コリジョン
		//==============================
		bricks.Intersects( &ball );
		Field::Intersects( &ball );
		paddle.Intersects( &ball );

		//==============================
		// 描画
		//==============================
		bricks.Draw();
		ball.Draw();
		paddle.Draw();
	}
}
