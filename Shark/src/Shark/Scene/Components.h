#pragma once

#include "Shark/Core/Base.h"
#include "Shark/Core/Concepts.h"
#include "Shark/Scene/Components/CoreComponents.h"
#include "Shark/Scene/Components/RendererComponents.h"
#include "Shark/Scene/Components/Physics2DComponents.h"
#include "Shark/Scene/Components/SceneComponents.h"

#include <ViennaTypeListLibrary/VTLL.h>

namespace Shark {

	namespace Components {

		using All = Group</* Core       */ IDComponent, TagComponent, TransformComponent, RelationshipComponent,
						  /* 2D         */ SpriteRendererComponent, CircleRendererComponent, TextRendererComponent,
						  /* 3D         */ MeshComponent, MeshFilterComponent, SubmeshComponent, StaticMeshComponent,
						  /* Prefab     */ PrefabComponent,
						  /* Light      */ PointLightComponent, DirectionalLightComponent, SkyComponent,
						  /* Camera     */ CameraComponent,
						  /* Physics 2D */ RigidBody2DComponent, BoxCollider2DComponent, CircleCollider2DComponent, DistanceJointComponent, HingeJointComponent, PrismaticJointComponent, PulleyJointComponent,
						  /* Script     */ ScriptComponent,
						  /* Audio      */ AudioComponent,
						  /* Animation  */ AnimationComponent>;

		
		// Group Core
		// defined in Scene/Components/Core.h

		// These Components are either hidden or immutable
		using UserHidden = Group<IDComponent, MeshFilterComponent, PrefabComponent>;
		using Automation = Group<SubmeshComponent, PrefabComponent, MeshFilterComponent, RelationshipComponent>;

		namespace detail {

			template<typename... Seqs>
			struct ExceptImpl;

			template<template <typename...> typename Seq1, template <typename...> typename Seq2, typename... Args>
			struct ExceptImpl<Seq1<Args...>, Seq2<>>
			{
				using type = Seq1<Args...>;
			};

			template<template <typename...> typename Seq1, typename... Args1, template <typename...> typename Seq2, typename... Args2>
			struct ExceptImpl<Seq1<Args1...>, Seq2<Args2...>>
			{
				using type = vtll::remove_types<Seq1<Args1...>, Seq2<Args2...>>;
			};

			template<template <typename...> typename Seq1, typename... Args1, typename Arg2>
			struct ExceptImpl<Seq1<Args1...>, Arg2>
			{
				using type = vtll::remove_types<Seq1<Args1...>, vtll::type_list<Arg2>>;
			};

			template<typename Seq1, typename Seq2, typename... Seq>
			struct ExceptImpl<Seq1, Seq2, Seq...>
			{
				using type0 = typename ExceptImpl<Seq1, Seq2>::type;
				using type = typename ExceptImpl<type0, Seq...>::type;
			};

		}

		template<typename... C>
		struct Group
		{
			template<typename Func>
			static constexpr auto Each(Func&& func)
			{
				Tuple::Each<C...>(std::forward<Func>(func));
			}

			template<typename Func>
			static constexpr auto EachIndexed(Func&& func)
			{
				Tuple::EachIndexed<C...>(std::forward<Func>(func));
			}


			template<typename... TOthers>
			using Combine = vtll::remove_duplicates<vtll::app<vtll::type_list<C...>, TOthers...>>;

			template<typename... Seqs>
			using Except = typename detail::ExceptImpl<vtll::type_list<C...>, Seqs...>::type;

			using ExceptCore = Except<Core>;
			using AsConstPointerTuple = std::tuple<const C*...>;

			template<template<typename...> typename T>
			using As = std::tuple<T<C>...>;
		};

	}

}
