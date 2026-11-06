# AI/SI Transparency and Disclosure

**NeoGeoSDK v1.7.7 pre-release documentation.**

Eagle Software supports clear, accurate and proportionate disclosure of how
development tools and games are made. Developers remain free to choose their
creative approach, workflow and quality standards, whether they use traditional
methods, AI/SI assistance or a combination of both.

## The SDK's development history

NeoGeoSDK's development history combines traditional programming with later,
selective AI/SI assistance. The SDK's foundational kernel, Artbox, the original
sound tools and the first driver implementations were developed using
traditional programming methods. AI/SI assistance was introduced later,
including work on test automation, the 2D engine, prototype games and their
artwork, and other supporting development tasks. Its involvement varies across
components and revisions.

Developers can write game code in C, C++ or Motorola 68000 assembly, use Z80
assembly for the sound driver, and use Python for development tools and
automation. They can create their own artwork, animation, audio and other
assets without using AI/SI tools in their own development workflow. Any
provenance statement should describe the relevant work accurately and
distinguish the SDK's history from the development of the game itself.

## Optional acknowledgement of the SDK's development history

**A statement about the SDK's AI/SI development history is optional when it
has not been requested and no applicable disclosure requirement calls for it.**

Under this policy, ordinary use of NeoGeoSDK does not automatically require a
game's README, credits or promotional material to include an AI/SI statement
about the SDK. A voluntary SDK credit may simply read "Built with NeoGeoSDK."

If a publisher, distribution platform, client or relevant authority requests
SDK provenance information, provide an accurate description appropriate to that
request and any applicable requirements. Describe the traditionally programmed
foundations and later AI/SI contributions in context, with reference to the
components and revision concerned where known.

## Disclosure of AI/SI used in the game's own development

**Any AI/SI use in the game's own development must be clearly disclosed.
This includes assistance as well as generated material.**

AI/SI tools here include generative models and assistants used for programming,
content creation, research or other development tasks. This rule covers the
team's use of those tools and known AI/SI-generated game content incorporated
from external sources. SDK provenance is addressed separately above.

When further details are requested or required by applicable rules, use the
following guidance for the content and presentation of the disclosure:

- **Describe the involvement accurately.** State which tools were used, what
  they were used for, and which parts of the project they affected. This covers
  planning, research, game design, code, debugging, testing, development tools,
  graphics, animation, audio, music, writing, translation, documentation and
  promotional material.
- **Disclose small contributions and later edits too.** AI/SI assistance still
  belongs in the disclosure when its output is edited, rewritten, redrawn or
  adapted. Include known AI/SI-generated material obtained from external sources.
- **Make the statement easy to find.** Include a clearly labelled AI/SI
  disclosure in the game's README, credits or accompanying documentation.
  Make it available with public development builds, demos, pre-releases and
  releases, and update it when the workflow changes.

## Example disclosures

Adapt these examples to the project's actual development history.

**Optional statement for a game developed without AI/SI tools or known
AI/SI-generated game content:**

> Our team did not use AI/SI tools or knowingly incorporate AI/SI-generated
> game content in this game's own development.

**Game developed with AI/SI assistance:**

> Our team used [tool name] to help generate test cases and diagnose selected
> code defects. The resulting work was reviewed and adapted by the team.

**SDK provenance statement, when requested or voluntarily included:**

> NeoGeoSDK's kernel and original foundations were developed traditionally.
> Later work on the 2D engine, test automation, prototype games and their
> artwork included AI/SI assistance. This SDK history is separate from our
> team's own development workflow.

## Evolving disclosure requirements

This policy may be revised as applicable AI/SI legislation, regulatory guidance
and distribution platform requirements evolve. Developers should check the
requirements relevant to their project, jurisdiction and distribution channels.
The optional SDK acknowledgement described here does not waive any disclosure
required by applicable rules. When additional information is requested during
a relevant review or publication process, disclosures should address that
request accurately and in context.

This document states Eagle Software's project transparency policy. It does
not amend the SDK's [MIT license](LICENSE).
