/*
	Scene
	mperron (2019)

	A base class from which all of the various screens in the game can
	be derived.
*/
class Scene : public Drawable {
protected:
	SDL_Texture *bg = NULL;

	list<Drawable*> drawables;
	list<Clickable*> clickables;

public:
	virtual ~Scene(){
		if(bg)
			SDL_DestroyTexture(bg);
	}

	virtual void draw(int ticks){
		// Draw the background image.
		if(bg)
			SDL_RenderCopy(rend, bg, NULL, NULL);

		// Draw any drawable elements (buttons, etc.)
		for(auto drawable : drawables)
			if(!drawable->drawable_hidden)
				drawable->draw(ticks);
	}

	virtual void check_mouse(SDL_Event event){
		for(auto clickable : clickables)
			clickable->check_mouse(event);
	}

	class Controller : public Drawable {
		Scene *scene_next = NULL;
		map<int, bool> *keys = NULL;
		list<Scene*> scene_stack;

	public:
		const int render_scale_max;

		Scene *scene = NULL;
		SDL_Window *win;
		bool fullscreen = false;

		Controller(SDL_Window *win, SDL_Renderer *rend, int &render_scale, map<int, bool> *keys) :
			Drawable(rend),
			render_scale_max(render_scale)
		{
			this->keys = keys;
			this->win = win;
		}

		SDL_Renderer *renderer(){
			return rend;
		}

		void set_scene(Scene *scene){
			this->scene_next = scene;
		}

		// Save the current scene onto the scene stack and descend into a new sub-scene.
		void scene_descend(Scene *scene){
			scene_stack.push_back(this->scene);

			set_scene(scene);
		}

		// Ascend from the current sub-scene to the parent scene on the stack.
		// Returns NULL if there is no previous scene on the stack.
		Scene *scene_ascend(){
			if(scene_stack.size() > 0){
				Scene *scene_next = scene_stack.back();

				scene_stack.pop_back();
				set_scene(scene_next);

				return scene_next;
			}

			return NULL;
		}

		void draw(int ticks){
			if(scene_next){
				if(scene){
					bool scene_on_stack = false;

					for(Scene *it : scene_stack){
						if(it == scene){
							scene_on_stack = true;
							break;
						}
					}

					// Delete the scene pointer iff it is not preserved in the scene_stack.
					if(!scene_on_stack)
						delete scene;
				}

				scene = scene_next;
				scene_next = NULL;
			}

			SDL_SetRenderDrawColor(rend, 0, 0, 0, 0xff);
			SDL_RenderClear(rend);

			if(scene)
				scene->draw(ticks);
		}

		void quit(){
			// Show a confirmation dialog?
			// TODO

			exit(0);
		}

		void check_mouse(SDL_Event event){
			if(scene)
				scene->check_mouse(event);
		}

		// Get the up/down state of a key. True if keydown.
		bool keystate(int keysym){
			return (*keys)[keysym];
		}
	};

protected:
	Controller *ctrl;

	Scene(Controller *ctrl) :
		Drawable(ctrl->renderer())
	{
		this->ctrl = ctrl;
	}

	class SceneFn {
	public:
		Scene* (*fn)(Scene::Controller*);

		SceneFn(Scene* (*fn)(Scene::Controller*)){
			this->fn = fn;
		}
	};

	static map<string, SceneFn*> scenes;

public:
	static void reg(string, Scene* (*fn)(Controller*));
	static Scene *create(Controller *ctrl, string);
};

map<string, Scene::SceneFn*> Scene::scenes;
void Scene::reg(string name, Scene* (*fn)(Scene::Controller*)){
	scenes[name] = new Scene::SceneFn(fn);
}
Scene *Scene::create(Scene::Controller *ctrl, string name){
	Scene::SceneFn *fn = scenes[name];

	if(fn)
		return fn->fn(ctrl);

	return NULL;
}

template<class T> Scene *scene_create(Scene::Controller *ctrl){
	return new T(ctrl);
}
