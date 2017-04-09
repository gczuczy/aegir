import { NgModule } from '@angular/core';
import { CommonModule } from '@angular/common';
import { RouterModule } from '@angular/router';
import { FormsModule } from '@angular/forms';

import { ProgramsComponent } from './programs.component';
import { AddProgramComponent } from './add-program.component';

import { PROGRAMS_ROUTES } from './programs.routes';

@NgModule({
    imports: [
	CommonModule,
	FormsModule,
	RouterModule.forChild(PROGRAMS_ROUTES)
    ],
    exports: [
	AddProgramComponent
    ],
    declarations: [ProgramsComponent, AddProgramComponent]
})
export class ProgramsModule { }
